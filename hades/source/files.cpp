#include "Hades/files.hpp"

#include <cassert>
#include <string>
#include <filesystem>

#include "SFML/System/FileInputStream.hpp"

#include "Hades/archive.hpp"
#include "Hades/Logging.hpp"
#include "Hades/StandardPaths.hpp"

namespace fs = std::experimental::filesystem;

namespace hades {
	namespace files {
		std::string as_string(const std::string &modPath, const std::string &fileName)
		{
			auto buf = as_raw(modPath, fileName);

			std::string out;
			//convert buff to str
			std::transform(std::begin(buf), std::end(buf), std::back_inserter(out), 
				[](auto i) { return static_cast<char>(i); });

			return out;
		}

		buffer as_raw(const std::string &modPath, const std::string &fileName)
		{
			auto stream = make_stream(modPath, fileName);
			auto size = stream.getSize();
			assert(size >= 0 && size <= std::numeric_limits<buffer::size_type>::max());
			buffer buff(static_cast<buffer::size_type>(size));
			stream.read(&buff[0], size);

			return buff;
		}

		FileStream make_stream(const std::string &modPath, const std::string &fileName)
		{
			static const auto custom_path = hades::GetUserCustomFileDirectory();

			try 
			{
				return std::move(FileStream(custom_path + modPath, fileName));
			}
			catch (file_exception&)
			{

				#ifndef NDEBUG
					return std::move(FileStream("../../game/" + modPath, fileName));
				#else
					return std::move(FileStream(modPath, filename));
				#endif
			}
		}

		FileStream::FileStream(const std::string &modPath, const std::string &fileName)
		{
			open(modPath, fileName);
		}

		FileStream::~FileStream()
		{
			if (file)
				_fileStream.~FileInputStream();
			else if (archive)
				_archiveStream.~archive_stream();
		}

		FileStream::FileStream(FileStream&& rhs) : file(rhs.file), archive(rhs.archive),
			_open(rhs._open), _mod_path(rhs._mod_path), _file_path(rhs._file_path)
		{
			if (file)
			{
				rhs._fileStream.~FileInputStream();
				new(&_fileStream) sf::FileInputStream();
				_fileStream.open(_mod_path + "/" + _file_path);
			}
			else if (archive)
			{
				new(&_archiveStream) zip::archive_stream(std::move(rhs._archiveStream));
				rhs._archiveStream.~archive_stream();
			}

			rhs.file = rhs.archive = rhs._open = false;
			rhs._mod_path.clear(); rhs._file_path.clear();
		}

		FileStream& FileStream::operator=(FileStream&& rhs)
		{
			if (file)
				_fileStream.~FileInputStream();
			else if (archive)
				_archiveStream.~archive_stream();

			file = rhs.file; archive = rhs.archive; _open = rhs._open;
			_mod_path = rhs._mod_path; _file_path = rhs._file_path;

			if (file)
			{
				rhs._fileStream.~FileInputStream();
				new(&_fileStream) sf::FileInputStream();
				_fileStream.open(_mod_path + "/" + _file_path);
			}
			else if (archive)
			{
				new(&_archiveStream) zip::archive_stream(std::move(rhs._archiveStream));
				rhs._archiveStream.~archive_stream();
			}

			rhs.file = rhs.archive = rhs._open = false;
			rhs._mod_path.clear(); rhs._file_path.clear();

			return *this;
		}

		void FileStream::open(const std::string &modPath, const std::string &fileName)
		{
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
						if (fs::is_directory(d))
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

					_mod_path = modPath;
					_file_path = fileName;

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

				_mod_path = archivepath;
				_file_path = fileName;
			}

			if (!found)
			{
				auto message = "Cannot find file: " + fileName + ",  in mod location: " + modPath;
				throw file_exception(message.c_str(), file_exception::error_code::FILE_NOT_FOUND);
			}

			assert(file != archive);
			_open = true;
		}

		sf::Int64 FileStream::read(void* data, sf::Int64 size)
		{
			assert(file || archive);

			if (!_open)
				throw std::logic_error("FileStream tried to call access function with file not open");

			if (file)
				return _fileStream.read(data, size);
			else
				return _archiveStream.read(data, size);
		}

		sf::Int64 FileStream::seek(sf::Int64 position)
		{
			assert(file || archive);

			if (!_open)
				throw std::logic_error("FileStream tried to call access function with file not open");

			if (file)
				return _fileStream.seek(position);
			else
				return _archiveStream.seek(position);
		}

		sf::Int64 FileStream::tell()
		{
			assert(file || archive);

			if (!_open)
				throw std::logic_error("FileStream tried to call access function with file not open");

			if (file)
				return _fileStream.tell();
			else
				return _archiveStream.tell();
		}

		sf::Int64 FileStream::getSize()
		{
			assert(file || archive);

			if (!_open)
				throw std::logic_error("FileStream tried to call access function with file not open");

			if (file)
				return _fileStream.getSize();
			else
				return _archiveStream.getSize();
		}

		std::vector<types::string> ListFilesInDirectory(types::string dir_path)
		{
			std::vector<types::string> output;

			fs::path dir(dir_path);
			if (!fs::is_directory(dir))
			{
				LOGERROR("\"" + dir_path + "\" is not a directory");
				return output;
			}

			for (auto &e : fs::directory_iterator(dir))
				output.push_back(e.path().filename().string());

			return output;
		}
	}
}
