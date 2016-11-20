#include "Hades/archive.hpp"

#include <cassert>
#include <limits>
#include <string>

#include "zlib/unzip.h"

namespace hades
{
	namespace zip
	{
		const char extension[] = ".zip";

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
			if (_fileOpen)
				unzCloseCurrentFile(_archive);

			if(_archive)
				close_archive(_archive); 
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

			buffer buff(size);

			assert(std::numeric_limits<unsigned int>::max() > size);
			auto amountRead = unzReadCurrentFile(_archive, &buff[0], size);

			memcpy(data, buff.data(), amountRead);

			return amountRead;
		}

		sf::Int64 archive_stream::seek(sf::Int64 position)
		{
			if (!_fileOpen)
				throw archive_exception("Tried to seek without an open file", archive_exception::error_code::FILE_NOT_OPEN);

			unzCloseCurrentFile(_archive);
			open(_fileName);

			buffer buff(position);

			return read(buff.data(), position);

			//TODO: loop, continue reading if the read wasnt far enough
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
			//test that archive exists
			//open archive
			path += extension;

			unarchive a = unzOpen(path.c_str());

			if (!a)
			{
				throw archive_exception((std::string("unable to open archive: ") + path.c_str()).c_str(), archive_exception::error_code::FILE_OPEN);
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
			buffer buff(size);
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
	}
}