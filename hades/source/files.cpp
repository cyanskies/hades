#include "Hades/files.hpp"

#include <cassert>
#include <string>
#include <filesystem>

#include "SFML/System/FileInputStream.hpp"

#include "Hades/archive.hpp"

namespace fs = std::experimental::filesystem;

namespace hades {
	namespace files {
		std::string as_string(const std::string &modPath, const std::string &fileName)
		{
			auto buf = as_raw(modPath, fileName);

			std::string out;
			//convert buff to str
			for (auto i : buf)
				out.push_back(static_cast<char>(i));

			return out;
		}

		buffer as_raw(const std::string &modPath, const std::string &fileName)
		{
			FileStream stream(modPath, fileName);

			auto size = stream.getSize();
			buffer buff(size);
			stream.read(&buff[0], size);

			return buff;
		}

		FileStream::FileStream(const std::string &modPath, const std::string &fileName)
		{
			//check that path points to a directory or archive
			std::error_code ec;
			if (!fs::exists(modPath, ec))
			{
				auto message = "Cannot find archive or directory: " + modPath;
				throw file_exception(message.c_str(), file_exception::error_code::PATH_NOT_FOUND);
			}

			//check if modPath exists as a archive or directory
			bool pathArchive = false, pathDirectory = false;
			std::string archiveExt;
			{
				fs::path parent = fs::path(modPath).parent_path();
				//iterate through directory and search for modpath.ext
				//and modpath/.
				fs::directory_iterator directory(parent);

				//get archive name
				auto namepos = modPath.find_last_of('/');
				auto archive_name = modPath.substr(namepos + 1, modPath.length() - namepos);

				for (auto d : directory)
				{
					if (d.path().stem() == archive_name)
					{
						if(fs::is_directory(d))
							pathDirectory = true;
						else
						{
							pathArchive = true;
							archiveExt = d.path().extension().string();
						}
					}

				}
			}

			bool found = false;
			//tryload from directory
			if (pathDirectory)
			{
				auto fullPath = modPath + '/' + fileName;
				if (fs::exists(fullPath))
				{
					found = true;
					file = true;
					//construct the fileinputstream union
					new(&_fileStream) sf::FileInputStream();

					if (!_fileStream.open(fullPath))
					{
						//cannot open, so this has failed
						//destruct filestream
						_fileStream.~FileInputStream();
						//throw error
						auto message = "Cannot open file: " + fileName + ",  in mod location: " + modPath;
						throw file_exception(message.c_str(), file_exception::error_code::UNREADABLE_FILE);
					}

					//file successfully opened, construction complete
				}
			}

			//tryload from archive
			if (!found && pathArchive)
			{
				auto archivepath = modPath + archiveExt;
				if (!zip::file_exists(archivepath, fileName))
				{
					auto message = "Cannot find file: " + fileName + ",  in mod archive: " + archivepath;
					throw file_exception(message.c_str(), file_exception::error_code::FILE_NOT_FOUND);
				}

				found = true;
				archive = true;
				//construct the fileinputstream union
				try
				{
					new(&_archiveStream) zip::archive_stream(archivepath);
				}
				catch (zip::archive_exception &a)
				{
					auto message = "Failed to open archive: " + archivepath + "." + a.what();
					throw file_exception(message.c_str(), file_exception::error_code::ARCHIVE_INVALID);
				}

				if (!_archiveStream.open(fileName))
				{
					auto message = "Cannot read file: " + fileName + ",  in mod archive: " + archivepath;
					throw file_exception(message.c_str(), file_exception::error_code::UNREADABLE_FILE);
				}
			}

			if (!found)
			{
				auto message = "Cannot find file: " + fileName +",  in mod location: " + modPath;
				throw file_exception(message.c_str(), file_exception::error_code::FILE_NOT_FOUND);
			}

			assert(file != archive);
		}

		FileStream::~FileStream()
		{
			assert(file || archive);
			if (file)
				_fileStream.~FileInputStream();
			else
				_archiveStream.~archive_stream();
		}

		sf::Int64 FileStream::read(void* data, sf::Int64 size)
		{
			assert(file || archive);

			if (file)
				return _fileStream.read(data, size);
			else
				return _archiveStream.read(data, size);
		}

		sf::Int64 FileStream::seek(sf::Int64 position)
		{
			assert(file || archive);

			if (file)
				return _fileStream.seek(position);
			else
				return _archiveStream.seek(position);
		}

		sf::Int64 FileStream::tell()
		{
			assert(file || archive);

			if (file)
				return _fileStream.tell();
			else
				return _archiveStream.tell();
		}

		sf::Int64 FileStream::getSize()
		{
			assert(file || archive);

			if (file)
				return _fileStream.getSize();
			else
				return _archiveStream.getSize();
		}
	}
}