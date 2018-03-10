#include "Hades/files.hpp"

#include <algorithm>
#include <cassert>
#include <string>
#include <experimental/filesystem>

#include "SFML/System/FileInputStream.hpp"

#include "Hades/archive.hpp"
#include "Hades/Logging.hpp"
#include "Hades/StandardPaths.hpp"

namespace fs = std::experimental::filesystem;

namespace hades {
	namespace files {
		std::string as_string(const std::string &modPath, const std::string &fileName)
		{
			const auto buf = as_raw(modPath, fileName);

			std::string out;
			//convert buff to str
			std::transform(std::begin(buf), std::end(buf), std::back_inserter(out),
				[](auto i) { return static_cast<char>(i); });

			return out;
		}

		buffer as_raw(const std::string &modPath, const std::string &fileName)
		{
			auto stream = make_stream(modPath, fileName);
			const auto size = stream.getSize();
			assert(size >= 0 && size <= std::numeric_limits<buffer::size_type>::max());
			buffer buff(static_cast<buffer::size_type>(size));
			stream.read(&buff[0], size);

			return buff;
		}

		ResourceStream make_stream(const std::string &modPath, const std::string &fileName)
		{
			const auto custom_path = hades::GetUserCustomFileDirectory();

			try
			{
				return std::move(ResourceStream(custom_path + modPath, fileName));
			}
			catch (file_exception&)
			{

				#ifndef NDEBUG
					return std::move(ResourceStream("../../game/" + modPath, fileName));
				#else
					return std::move(ResourceStream(modPath, filename));
				#endif
			}
		}

		ResourceStream::ResourceStream(const std::string &modPath, const std::string &fileName)
		{
			open(modPath, fileName);
		}

		void ResourceStream::open(const std::string &modPath, const std::string &fileName)
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
				const auto namepos = modPath.find_last_of('/');
				const auto archive_name = modPath.substr(namepos + 1, modPath.length() - namepos);

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

					File fstream;
					if (!fstream.open(fullPath))
					{
						const auto message = "Cannot open file: " + fileName + ",  in mod location: " + modPath;
						throw file_exception(message.c_str(), file_exception::error_code::UNREADABLE_FILE);
					}

					_stream = std::move(fstream);

					_mod_path = modPath;
					_file_path = fileName;

					//file successfully opened, construction complete
				}
			}

			//tryload from archive
			if (!found && pathArchive)
			{
				const auto archivepath = modPath + archiveExt;
				if (!zip::file_exists(archivepath, fileName))
				{
					const auto message = "Cannot find file: " + fileName + ",  in mod archive: " + archivepath;
					throw file_exception(message.c_str(), file_exception::error_code::FILE_NOT_FOUND);
				}

				found = true;

				try
				{
					zip::archive_stream astream{ archivepath };
					if (!astream.open(fileName))
					{
						const auto message = "Cannot read file: " + fileName + ",  in mod archive: " + archivepath;
						throw file_exception(message.c_str(), file_exception::error_code::UNREADABLE_FILE);
					}

					_stream = std::move(astream);
				}
				catch (zip::archive_exception &a)
				{
					const auto message = "Failed to open archive: " + archivepath + "." + a.what();
					throw file_exception(message.c_str(), file_exception::error_code::ARCHIVE_INVALID);
				}

				_mod_path = archivepath;
				_file_path = fileName;
			}

			if (!found)
			{
				const auto message = "Cannot find file: " + fileName + ",  in mod location: " + modPath;
				throw file_exception(message.c_str(), file_exception::error_code::FILE_NOT_FOUND);
			}

			_open = true;
		}

		sf::Int64 ResourceStream::read(void* data, sf::Int64 size)
		{
			if (!_open)
				throw std::logic_error("ResourceStream tried to call access function with file not open");

			return std::visit([data, size](auto && s) {
				return s.read(data, size);
			}, _stream);
		}

		sf::Int64 ResourceStream::seek(sf::Int64 position)
		{
			if (!_open)
				throw std::logic_error("ResourceStream tried to call access function with file not open");

			return std::visit([position](auto && s) {
				return s.seek(position);
			}, _stream);
		}

		sf::Int64 ResourceStream::tell()
		{
			if (!_open)
				throw std::logic_error("ResourceStream tried to call access function with file not open");

			return std::visit([](auto && s) {
				return s.tell();
			}, _stream);
		}

		sf::Int64 ResourceStream::getSize()
		{
			if (!_open)
				throw std::logic_error("ResourceStream tried to call access function with file not open");

			return std::visit([](auto && s) {
				return s.getSize();
			}, _stream);
		}

		std::vector<types::string> ListFilesInDirectory(types::string dir_path)
		{
			std::vector<types::string> output;

			const fs::path dir(dir_path);
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
