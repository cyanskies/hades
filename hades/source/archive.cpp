#include "Hades/archive.hpp"

#include <cassert>
#include <limits>
#include <string>

#include "zlib/unzip.h"

namespace hades
{
	namespace zip
	{
		archive_exception::archive_exception(const char* what, error_code code)
			: _what(what), _code(code)
		{}

		struct unarchive
		{
			unzFile file;
		};
		
		archive_stream::archive_stream(std::string archive) : _fileOpen(false), _archivePath(archive)
		{
			_archive = open_archive(archive);
		}

		archive_stream::~archive_stream() 
		{
			if (_fileOpen)
				unzCloseCurrentFile(_archive);

			close_archive(&_archive); 
		}

		bool archive_stream::open(std::string filename)
		{
			if(_fileOpen)
				throw archive_exception("This stream already has a file open", archive_exception::error_code::FILE_ALREADY_OPEN);

			if (file_exists(_archive, filename))
				return false;

			//open current file for reading
			auto r = unzOpenCurrentFile(_archive);
			if (r != UNZ_OK)
				return false;

			_fileOpen = true;
			_fileName = filename;

			return _fileOpen;
		}

		sf::Int64 archive_stream::read(void* data, sf::Int64 size)
		{
			assert(_fileOpen && data);
			buffer buff(size);

			assert(std::numeric_limits<unsigned int>::max() > size);
			auto amountRead = unzReadCurrentFile(_archive, &buff[0], size);

			memcpy(data, buff.data(), amountRead);

			return amountRead;
		}

		sf::Int64 archive_stream::seek(sf::Int64 position)
		{
			assert(_fileOpen);

			unzCloseCurrentFile(_archive);
			open(_fileName);

			buffer buff(position);

			return read(buff.data(), position);
		}

		sf::Int64 archive_stream::tell()
		{
			assert(_fileOpen);

			return unztell64(_archive);
		}

		sf::Int64 archive_stream::getSize()
		{
			assert(_fileOpen);
			unz_file_info64 info;

			auto r = unzGetCurrentFileInfo64(_archive, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			assert(r == UNZ_OK);

			return info.uncompressed_size;
		}

		unarchive open_archive(std::string path)
		{
			//test that archive exists
			//open archive
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

		const int case_sensitivity_auto = 0,
			case_sensitivity_sensitive = 1,
			case_sensitivity_none = 2;

		buffer read_file_from_archive(unarchive a, std::string path)
		{
			return buffer();
		}

		bool file_exists(std::string archive, std::string path)
		{
			auto a = open_archive(archive);
			return file_exists(a, path);
		}

		bool file_exists(unarchive a, std::string path)
		{
			auto r = unzLocateFile(a, path.c_str(), case_sensitivity_auto);
			return r == UNZ_OK;
		}
	}
}