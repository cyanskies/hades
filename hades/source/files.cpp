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
		types::string as_string(std::string_view modPath, std::string_view fileName)
		{
			const auto buf = as_raw(modPath, fileName);

			types::string out;
			//convert buff to str
			std::transform(std::begin(buf), std::end(buf), std::back_inserter(out),
				[](auto i) { return static_cast<char>(i); });

			return out;
		}

		buffer as_raw(std::string_view modPath, std::string_view fileName)
		{
			auto stream = make_stream(modPath, fileName);
			const auto size = stream.getSize();
			assert(size >= 0 && size <= std::numeric_limits<buffer::size_type>::max());
			buffer buff(static_cast<buffer::size_type>(size));
			stream.read(&buff[0], size);

			return buff;
		}

		ResourceStream make_stream(std::string_view modPath, std::string_view fileName)
		{
			const auto custom_path = hades::GetUserCustomFileDirectory();

			try
			{
				return std::move(ResourceStream(custom_path + to_string(modPath), fileName));
			}
			catch (file_exception&)
			{

				#ifndef NDEBUG
					return std::move(ResourceStream("../../game/" + to_string(modPath), fileName));
				#else
					return std::move(ResourceStream(modPath, filename));
				#endif
			}
		}

		ResourceStream::ResourceStream(std::string_view modPath, std::string_view fileName)
		{
			open(modPath, fileName);
		}

		void ResourceStream::open(std::string_view modPath, std::string_view fileName)
		{
			//check if modPath exists as a archive or directory
			bool pathArchive = false, pathDirectory = false;
			types::string archiveExt;
			const types::string mod{ modPath };
			const types::string file{ fileName };
			{
				fs::path parent = fs::path{ mod }.parent_path();
				//iterate through directory and search for modpath.ext
				//and modpath/.
				fs::directory_iterator directory{ parent };

				//get archive name
				const auto namepos = mod.find_last_of('/');
				const auto archive_name = mod.substr(namepos + 1, mod.length() - namepos);

				for (auto d : directory)
				{
					if (d.path().stem().string() == archive_name)
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
				const auto fullPath = mod + '/' + file;
				if (fs::exists(fullPath))
				{
					found = true;

					File fstream;
					if (!fstream.open(fullPath))
					{
						const auto message = "Cannot open file: " + file + ",  in mod location: " + mod;
						throw file_exception(message.c_str(), file_exception::error_code::UNREADABLE_FILE);
					}

					_stream = std::move(fstream);

					_mod_path = mod;
					_file_path = file;

					//file successfully opened, construction complete
				}
			}

			//tryload from archive
			if (!found && pathArchive)
			{
				const auto archivepath = mod + archiveExt;
				if (!zip::file_exists(archivepath, file))
				{
					const auto message = "Cannot find file: " + file + ",  in mod archive: " + archivepath;
					throw file_exception(message.c_str(), file_exception::error_code::FILE_NOT_FOUND);
				}

				found = true;

				try
				{
					zip::archive_stream astream{ archivepath };
					if (!astream.open(file))
					{
						const auto message = "Cannot read file: " + file + ",  in mod archive: " + archivepath;
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
				_file_path = file;
			}

			if (!found)
			{
				const auto message = "Cannot find file: " + file + ",  in mod location: " + mod;
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

		std::vector<types::string> ListFilesInDirectory(std::string_view dir_path)
		{
			std::vector<types::string> output;

			const fs::path dir(to_string(dir_path));
			if (!fs::is_directory(dir))
			{
				LOGERROR("\"" + to_string(dir_path) + "\" is not a directory");
				return output;
			}

			for (auto &e : fs::directory_iterator(dir))
				output.push_back(e.path().filename().string());

			return output;
		}
	}
}
