#include "Hades/archive.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

#include "zlib/zip.h"
#include "zlib/unzip.h"

#include "Hades/Logging.hpp"
#include "Hades/Types.hpp"

namespace fs = std::experimental::filesystem;

//this is the default archive extension
//used when created archives, but not when searching for them
const hades::types::string archive_ext = ".zip";

namespace hades
{
	namespace zip
	{
		archive_exception::archive_exception(const char* what, error_code code)
			: std::exception(what), _code(code)
		{}

		//open a close unzip archives
		unarchive open_archive(types::string path);
		void close_archive(unarchive f);

		archive_stream::archive_stream(types::string archive) : _fileOpen(false), _archive(open_archive(archive))
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

		bool file_exists(unarchive, types::string);

		bool archive_stream::open(types::string filename)
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

		unarchive open_archive(types::string path)
		{
			if(!fs::exists(path))
				throw archive_exception(("Archive not found: " + path).c_str(), archive_exception::error_code::FILE_NOT_FOUND);
			//open archive
			unarchive a = unzOpen(path.c_str());

			if (!a)
			{
				throw archive_exception(("Unable to open archive: " + path).c_str(), archive_exception::error_code::FILE_OPEN);
			}

			return a;
		}

		void close_archive(unarchive f)
		{
			assert(f);

			auto r = unzClose(f);

			if(r != UNZ_OK)
				throw archive_exception("Unable to close archive", archive_exception::error_code::FILE_CLOSE);
		}

		buffer read_file_from_archive(types::string archive, types::string path)
		{
			auto stream = stream_file_from_archive(archive, path);
				
			auto size = stream.getSize();
			
			assert(std::numeric_limits<unsigned int>::max() > size);
			buffer buff(static_cast<buffer::size_type>(size));
			stream.read(&buff[0], size);

			return buff;
		}

		types::string read_text_from_archive(types::string archive, types::string path)
		{
			auto buf = read_file_from_archive(archive, path);

			types::string out;
			//convert buff to str
			for (auto i : buf)
				out.push_back(static_cast<char>(i));

			return out;
		}

		archive_stream stream_file_from_archive(types::string archive, types::string path)
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

		bool file_exists(types::string archive, types::string path)
		{
			auto a = open_archive(archive);
			auto ret = file_exists(a, path);
			close_archive(a);
			return ret;
		}

		bool file_exists(unarchive a, types::string path)
		{
			auto r = unzLocateFile(a, path.c_str(), case_sensitivity_auto);
			return r == UNZ_OK;
		}

		types::uint16 count_separators(types::string s)
		{
			const char separator1 = '\\';
			const char separator2 = '/';

			auto sep_count = std::count(s.begin(), s.end(), separator1);
			return sep_count + std::count(s.begin(), s.end(), separator2);
		}

		std::vector<types::string> list_files_in_archive(types::string archive, types::string dir_path, bool recursive)
		{
			std::vector<types::string> output;

			auto zip = open_archive(archive);

			auto ret = unzGoToFirstFile(zip);
			if (ret != ZIP_OK)
				throw archive_exception(("Error finding file in archive: " + archive).c_str(), archive_exception::error_code::FILE_OPEN);

			auto separator_count = count_separators(dir_path);
			
			//while theirs more files
			while (unzGoToNextFile(zip) != UNZ_END_OF_LIST_OF_FILE)
			{
				unz_file_info info;
				unzGetCurrentFileInfo(zip, &info, nullptr, 0, nullptr, 0, nullptr, 0);

				using char_buffer = std::vector<char>;
				char_buffer name(info.size_filename);

				unzGetCurrentFileInfo(zip, &info, &name[0], info.size_filename, nullptr, 0, nullptr, 0);
				
				types::string file_name(name.begin(), name.end());

				if (dir_path.empty())
				{
					//list file if it has no directory seperator or more if recursive
					if(recursive || count_separators(file_name) == 0)
						output.push_back(file_name);
				}
				else
				{
					//list file if it contains the same number of directory seperators as dir_path or more if recursive
					if (recursive && count_separators(file_name) >= separator_count)
						output.push_back(file_name);
					else if (count_separators(file_name) == separator_count)
						output.push_back(file_name.substr(dir_path.length(), file_name.length() - dir_path.length()));
				}
			}

			close_archive(zip);

			return output;
		}

		void compress_directory(types::string path)
		{
			if (!fs::is_directory(path))
				throw archive_exception(("Path is not a directory: " + path).c_str(), archive_exception::error_code::FILE_NOT_FOUND);

			if (!fs::exists(path))
				throw archive_exception(("Directory not found: " + path).c_str(), archive_exception::error_code::FILE_NOT_FOUND);

			//overwrite if the file already exists
			if (fs::exists(path + ".zip"))
			{
				LOGWARNING("Archive already exists overwriting: " + path + ".zip");
				std::error_code errorc;
				if(!fs::remove(path + ".zip", errorc))
					archive_exception(("Unable to modify existing archive. Reason: " + errorc.message()).c_str(), archive_exception::error_code::FILE_WRITE);
			}
		
			types::string new_zip_path;

			fs::path fs_path(path);
			auto root = fs_path.parent_path();
			auto name = --fs_path.end();

			//detect likely bug
			//may need to do --(--fs_path.end())
			assert(*name != ".");

			auto zip = zipOpen((root.generic_string() + "/" + name->generic_string() + archive_ext).c_str(), APPEND_STATUS_CREATE);

			assert(zip);

			if(!zip)
				throw archive_exception(("Error creating zip file: " + name->generic_string()).c_str(), archive_exception::error_code::FILE_WRITE);

			LOG("Created archive: " + name->generic_string());

			//for each file in dir
			//create new zip file
			for (auto &f : fs::recursive_directory_iterator(fs_path))
			{
				auto p = f.path();
				if (fs::is_directory(p))
					continue;

				auto file_path = p.string();
				auto directory = fs_path.string();
				if (file_path.find(directory) == types::string::npos)
					throw archive_exception("Directory path isn't a subset of the file path, unexpected error", archive_exception::error_code::FILE_READ);


				auto file_name = file_path.substr(directory.length(), file_path.length() - directory.length());
				
				//remove leading /
				file_name.erase(file_name.begin());
				
				auto date = fs::last_write_time(p);
				auto t = date.time_since_epoch();

				//NOTE: if this is firing, then we need to enable zip64 compression below
				if (fs::file_size(p) > 0xffffffff)
					throw archive_exception(("Cannot store file in archive: " + file_name + " file too large").c_str(), archive_exception::error_code::FILE_WRITE);

				zip_fileinfo file_info{ tm_zip{}, 0, 0, 0 };
				auto ret = zipOpenNewFileInZip(zip, file_name.c_str(), &file_info, nullptr, 0, nullptr, 0, nullptr,
					Z_DEFLATED, // 0 = store, Z_DEFLATE = deflate
					Z_DEFAULT_COMPRESSION);

				if (ret != ZIP_OK)
					throw archive_exception(("Error writing file: " + p.filename().generic_string() + " to archive: " + name->generic_string()).c_str(), archive_exception::error_code::FILE_WRITE);

				using char_buffer = std::vector<char>;

				//NOTE:, the buffer size will have to be double checked if we move to zip64 compression
				std::ifstream in(p.string(), std::ios::binary | std::ios::ate);
				auto size = in.tellg();			

				//if the file is size 0(like many git tag files are)
				//then skip this whole thing and close the file.
				if (size > 0)
				{
					char_buffer buf(static_cast<char_buffer::size_type>(size));
					in.seekg(std::ios::beg);
					in.read(&buf[0], size);

					//NOTE: same as for the buffer, size is being truncated if larger than 32 bits
					ret = zipWriteInFileInZip(zip, &buf[0], static_cast<unsigned int>(size.seekpos()));
					if (ret != ZIP_OK)
						throw archive_exception(("Error writing file: " + p.filename().generic_string() + " to archive: " + name->generic_string()).c_str(), archive_exception::error_code::FILE_OPEN);
				}

				LOG("Wrote " + file_name + " to archive: " + name->generic_string());

				ret = zipCloseFileInZip(zip);
				if (ret != ZIP_OK)
					throw archive_exception(("Error writing file: " + p.filename().generic_string() + " to archive: " + name->generic_string()).c_str(), archive_exception::error_code::FILE_OPEN);
			}
			
			auto ret = zipClose(zip, nullptr);
			if (ret != ZIP_OK)
				throw archive_exception(("Error closing file: " + name->generic_string()).c_str(), archive_exception::error_code::FILE_CLOSE);

			LOG("Completed creating archive: " + name->generic_string());
		}

		void uncompress_archive(types::string path)
		{
			//confirm is archive
			fs::path fs_path(path);
			if (!fs::exists(fs_path))
				throw archive_exception(("Cannot open archive, file not found: " + path).c_str(), archive_exception::error_code::FILE_NOT_FOUND);

			//create directory if absent
			auto root_dir = fs_path.parent_path() / fs_path.stem();

			auto archive = open_archive(path);

			auto ret = unzGoToFirstFile(archive);
			if (ret != ZIP_OK)
				throw archive_exception(("Error finding file in archive: " + path).c_str(), archive_exception::error_code::FILE_OPEN);

			LOG("Uncompressing archive: " + path);

			//for each file in directory
			do{
				unz_file_info info;
				unzGetCurrentFileInfo(archive, &info, nullptr, 0, nullptr, 0, nullptr, 0);

				using char_buffer = std::vector<char>;
				char_buffer name(info.size_filename);

				ret = unzGetCurrentFileInfo(archive, &info, &name[0], info.size_filename, nullptr, 0, nullptr, 0);
				if (ret != ZIP_OK)
					throw archive_exception("Error reading file info from archive", archive_exception::error_code::FILE_OPEN);

				types::string filename(name.begin(), name.end());

				fs::path parent_dir(filename);
				//create directory if it dosn't already exist
				fs::create_directories(root_dir / parent_dir.parent_path()); //only creates missing directories

				//open file
				std::ofstream file(root_dir.string() + "/" + filename, std::ios::binary | std::ios::trunc);	

				if (!file.is_open())
					throw archive_exception(("Failed to create or open file: " + filename).c_str(), archive_exception::error_code::FILE_WRITE);

				//write data to file
				auto size = info.uncompressed_size;

				//if this is being triggered then we need to upgrade to zip64
				if (std::numeric_limits<unsigned int>::max() < info.uncompressed_size)
					throw archive_exception(("Cannot extract file from archive: " + filename + " file too large").c_str(), archive_exception::error_code::FILE_WRITE);

				auto usize = static_cast<unsigned int>(size);
				using char_buffer = std::vector<char>;
				char_buffer buff(static_cast<buffer::size_type>(usize));

				ret = unzOpenCurrentFile(archive);
				if (ret != ZIP_OK)
					throw archive_exception(("Failed to open file in archive: " + path + filename).c_str(), archive_exception::error_code::FILE_OPEN);

				types::uint32 total_written = 0;

				LOG("Uncompressing file: " + filename + " from archive: " + path);

				//write the data to actual files
				while (total_written < usize)
				{
					auto amount = unzReadCurrentFile(archive, &buff[0], usize);
					file.write(buff.data(), amount);
					total_written += amount;
				}

				ret = unzCloseCurrentFile(archive);
				if (ret != ZIP_OK)
					throw archive_exception(("Failed to close file in archive: " + path + filename).c_str(), archive_exception::error_code::FILE_CLOSE);
			} while (unzGoToNextFile(archive) != UNZ_END_OF_LIST_OF_FILE);
			
			close_archive(archive);

			LOG("Finished uncompressing archive: " + path);
		}
	}
}
