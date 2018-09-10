#include "Hades/files.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <string>
#include <filesystem>

#include "SFML/System/FileInputStream.hpp"

#include "hades/archive.hpp"
#include "hades/logging.hpp"
#include "hades/standard_paths.hpp"
#include "hades/utility.hpp"

namespace fs = std::filesystem;

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
			//TODO: use inflate if needed(see probably_compressed)
			auto stream = make_stream(modPath, fileName);
			const auto size = stream.getSize();
			assert(size >= 0 && size <= std::numeric_limits<buffer::size_type>::max());
			buffer buff(static_cast<buffer::size_type>(size));
			stream.read(&buff[0], size);

			if (zip::probably_compressed(buff))
				buff = zip::inflate(buff);

			return buff;
		}

		buffer read_file(std::string_view path)
		{
			std::ifstream file(path, std::ios_base::binary | std::ios_base::in);

			buffer buf{};
			while (!file.eof())
				buf.push_back(static_cast<std::byte>(file.get()));

			if (zip::probably_compressed(buf))
				return zip::inflate(buf);

			return buf;
		}

		types::string read_file_string(std::string_view path)
		{
			const auto buffer = read_file(path);

			types::string out;
			std::transform(std::begin(buffer), std::end(buffer), std::back_inserter(out), [](const std::byte b) {
				return static_cast<types::string::value_type>(b);
			});

			return out;
		}

		types::string try_read(std::string_view first_path, std::string_view second_path, std::string_view file_name)
		{
			const auto first = read_file_string(to_string(first_path) + to_string(file_name));

			if (!first.empty())
				return first;

			const auto second = read_file_string(to_string(second_path) + to_string(file_name));

			if (!second.empty())
				return second;

			const auto message = "Unable to find file: " + to_string(file_name);
			throw file_exception{message.c_str(), file_exception::error_code::FILE_NOT_FOUND};
		}

		types::string read_save(std::string_view file_name)
		{
			return try_read(hades::user_save_directory(), standard_save_directory(), file_name);
		}

		types::string read_config(std::string_view file_name)
		{
			return try_read(hades::user_config_directory(), standard_config_directory(), file_name);
		}

		ResourceStream make_stream(std::string_view modPath, std::string_view fileName)
		{
			const auto custom_path = hades::user_custom_file_directory();

			//TODO: make another resource stream constructor,
			//so that we can do this without throwing unless there is actually an error
			try
			{
				return ResourceStream(custom_path + to_string(modPath), fileName);
			}
			catch (const file_exception&)
			{
				#ifndef NDEBUG
					return ResourceStream("../../game/" + to_string(modPath), fileName);
				#else
					return ResourceStream(modPath, fileName);
				#endif
			}
		}

		bool make_directory(fs::path dir)
		{
			if (fs::exists(dir) && fs::is_directory(dir))
				return true;

			//TODO: return false if mising write permissions for the directory that exists

			return fs::create_directories(dir);
		}

		void write_file(std::string_view path, std::string_view file_contents)
		{
			const auto userCustomFileDirectory = hades::user_custom_file_directory();
			const auto target = userCustomFileDirectory + to_string(path);

			fs::path p{ target };
			if (!p.has_filename())
			{
				const auto message = "unable to write file, path didn't include a filename; path was: " + p.u8string();
				throw file_exception(message.c_str(), file_exception::error_code::FILENAME_INVALID);
			}

			auto parent = p.parent_path();
			if (!make_directory(parent))
			{
				const auto message = "No write permission for directory or unable to create directory; was: " + parent.u8string();
				throw file_exception(message.c_str(), file_exception::error_code::PERMISSION_DENIED);
			}

			buffer as_bytes{};

			std::transform(std::begin(file_contents), std::end(file_contents), std::back_inserter(as_bytes), [](const auto c) {
				return static_cast<std::byte>(c);
			});

			buffer output = zip::deflate(as_bytes);			

			std::ofstream file{ p, std::ios::binary | std::ios::trunc };
			if (file.is_open())
				file.write(reinterpret_cast<const char*>(output.data()), output.size());
			else
				//todo throw instead
				LOGERROR("Failed to open file for writing: " + target);
		}

		ResourceStream::ResourceStream(std::string_view modPath, std::string_view fileName)
		{
			open(modPath, fileName);
		}

		//acceptable zip extensions
		//zip == standard zip
		//? hdf == hades data file
		//? data == data file
		constexpr auto extensions = std::array{ "zip" };

		void ResourceStream::open(std::string_view modPath, std::string_view fileName)
		{
			//check if modPath exists as a archive or directory
			bool pathDirectory = false;
			types::string pathArchive{};
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
					if (d.path().stem() == archive_name)
					{
						if (fs::is_directory(d))
							pathDirectory = true;
						else if(std::any_of(std::begin(extensions), std::end(extensions),
							[ext = d.path().extension()](auto &&other){ return ext == other; }))
						{
							pathArchive = d.path().string();
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
			if (!found && !pathArchive.empty())
			{
				const auto archivepath = pathArchive;
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
