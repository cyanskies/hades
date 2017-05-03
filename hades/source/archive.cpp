#include "Hades/archive.hpp"

#include <cassert>
#include <filesystem>
#include <limits>
#include <string>

#include "zlib/zip.h"
#include "zlib/unzip.h"

#include "Hades/Logging.hpp"

namespace fs = std::experimental::filesystem;

namespace hades
{
	namespace zip
	{
		archive_exception::archive_exception(const char* what, error_code code)
			: std::exception(what), _code(code)
		{}

		//open a close unzip archives
		unarchive open_archive(std::string path);
		void close_archive(unarchive f);

		archive_stream::archive_stream(std::string archive) : _fileOpen(false), _archive(open_archive(archive))
		{}

		archive_stream::archive_stream(archive_stream&& rhs) : _fileOpen(rhs._fileOpen),
			_archive(rhs._archive), _fileName(rhs._fileName)
		{
			rhs._archive = nullptr;
			rhs._fileOpen = false;
		}

		archive_stream &archive_stream::operator=(archive_stream&& rhs)
		{
			_archive = rhs._archive;
			rhs._archive = nullptr;
			_fileOpen = rhs._fileOpen;
			_fileOpen = false;
			_fileName = std::move(rhs._fileName);

			return *this;
		}

		archive_stream::~archive_stream()
		{
			//use zlib directly, since our wrappers might throw
			if (_fileOpen)
				unzCloseCurrentFile(_archive);

			if (_archive)
				unzClose(_archive);
		}

		bool file_exists(unarchive, std::string);

		bool archive_stream::open(std::string filename)
		{
			if(_fileOpen)
				throw archive_exception("This stream already has a file open", archive_exception::error_code::FILE_ALREADY_OPEN);

			if (!file_exists(_archive, filename))
				return false;

			//open current file for reading
			auto r = unzOpenCurrentFile(_archive);
			if (r != UNZ_OK)
				return false;

			_fileOpen = true;
			_fileName = filename;

			return _fileOpen;
		}

		bool archive_stream::is_open() const
		{
			return _fileOpen;
		}

		sf::Int64 archive_stream::read(void* data, sf::Int64 size)
		{
			assert(data);
			if(!_fileOpen)
				throw archive_exception("Tried to read without an open file", archive_exception::error_code::FILE_NOT_OPEN);

			//unsigned int type used by zlib
			auto usize = static_cast<unsigned int>(size);

			buffer buff(static_cast<buffer::size_type>(usize));

			//NOTE: uint32 max worth of bytes is around 4gb, so we shouldn't have
			// a problem unless someones trying to load something really big, but then the 
			// assert should catch it.
			assert(std::numeric_limits<unsigned int>::max() > size);
			auto amountRead = unzReadCurrentFile(_archive, &buff[0], usize);

			memcpy(data, buff.data(), amountRead);

			return amountRead;
		}

		sf::Int64 archive_stream::seek(sf::Int64 position)
		{
			if (!_fileOpen)
				throw archive_exception("Tried to seek without an open file", archive_exception::error_code::FILE_NOT_OPEN);

			//close and open the file to reset the seek ptr
			if (unzCloseCurrentFile(_archive) == UNZ_CRCERROR)
			{
				//file closed but crc check indicates an error
				LOGWARNING("CRC error in archive: " + _fileName);
			}

			_fileOpen = false;

			if(!open(_fileName))
				throw archive_exception(("Failed to open file in archive: " + _fileName).c_str(), archive_exception::error_code::FILE_OPEN);

			assert(std::numeric_limits<unsigned int>::max() > position);
			buffer buff(static_cast<buffer::size_type>(position));

			sf::Int64 total = 0;

			while(total < position)
				total += read(buff.data(), position);

			return total;
		}

		sf::Int64 archive_stream::tell()
		{
			if (!_fileOpen)
				throw archive_exception("Tried to tell without an open file", archive_exception::error_code::FILE_NOT_OPEN);

			return unztell64(_archive);
		}

		sf::Int64 archive_stream::getSize()
		{
			if (!_fileOpen)
				throw archive_exception("Tried to get size without an open file", archive_exception::error_code::FILE_NOT_OPEN);

			unz_file_info64 info;

			auto r = unzGetCurrentFileInfo64(_archive, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			assert(r == UNZ_OK);

			return info.uncompressed_size;
		}

		unarchive open_archive(std::string path)
		{
			if(!fs::exists(path))
				throw archive_exception(("archive not found: " + path).c_str(), archive_exception::error_code::FILE_OPEN);
			//open archive
			unarchive a = unzOpen(path.c_str());

			if (!a)
			{
				throw archive_exception(("unable to open archive: " + path).c_str(), archive_exception::error_code::FILE_OPEN);
			}

			return a;
		}

		void close_archive(unarchive f)
		{
			assert(f);

			auto r = unzClose(f);

			if(r != UNZ_OK)
				throw archive_exception("unable to close archive", archive_exception::error_code::FILE_CLOSE);
		}

		buffer read_file_from_archive(std::string archive, std::string path)
		{
			auto stream = stream_file_from_archive(archive, path);
				
			auto size = stream.getSize();
			
			assert(std::numeric_limits<unsigned int>::max() > size);
			buffer buff(static_cast<buffer::size_type>(size));
			stream.read(&buff[0], size);

			return buff;
		}

		std::string read_text_from_archive(std::string archive, std::string path)
		{
			auto buf = read_file_from_archive(archive, path);

			std::string out;
			//convert buff to str
			for (auto i : buf)
				out.push_back(static_cast<char>(i));

			return out;
		}

		archive_stream stream_file_from_archive(std::string archive, std::string path)
		{
			archive_stream str(archive);
			str.open(path);
			return str;
		}

		//no sensitivity on windows
		const int case_sensitivity_auto = 0,
			//case sensitivity everywhere
			case_sensitivity_sensitive = 1,
			//no sensitivity on any platform
			case_sensitivity_none = 2;

		bool file_exists(std::string archive, std::string path)
		{
			auto a = open_archive(archive);
			auto ret = file_exists(a, path);
			close_archive(a);
			return ret;
		}

		bool file_exists(unarchive a, std::string path)
		{
			auto r = unzLocateFile(a, path.c_str(), case_sensitivity_auto);
			return r == UNZ_OK;
		}

		bool compress_directory(std::string path)
		{
			if (!fs::is_directory(path))
				throw archive_exception(("path is not a directory: " + path).c_str(), archive_exception::error_code::FILE_OPEN);

			if (!fs::exists(path))
				throw archive_exception(("directory not found: " + path).c_str(), archive_exception::error_code::FILE_OPEN);

			types::string new_zip_path;

			fs::path fs_path(path);
			auto root = fs_path.parent_path();
			auto name = --fs_path.end();

			//detect likely bug
			//may need to do --(--fs_path.end())
			assert(*name != ".");

			auto zip = zipOpen((root.generic_string() + "/" + name->generic_string()).c_str(), APPEND_STATUS_CREATE);

			//TODO: throw on error
			assert(zip);
			return false;
		}

		bool uncompress_archive(std::string path)
		{
			//TODO: impliment
			return false;
		}
	}
}