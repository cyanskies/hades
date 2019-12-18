#include "hades/archive.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring> // for std::memcopy
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

#include "zip.h"
#include "unzip.h"

#undef ERROR

#include "hades/console_variables.hpp"
#include "hades/logging.hpp"
#include "hades/properties.hpp"
#include "hades/types.hpp"

//TODO: revert to std::filesystem once support comes in both MSVC and GCC
namespace fs = std::filesystem;

//this is the default archive extension
//used when created archives, but not when searching for them
constexpr auto archive_ext = ".zip";

namespace hades::zip
{
	//open a close unzip archives
	unarchive open_archive(const std::filesystem::path&);
	void close_archive(unarchive f);

	static bool file_exists(unarchive, const fs::path&);

	template<typename Integer>
	static unsigned int CheckSizeLimits(Integer size)
	{
		if constexpr (std::is_signed_v<Integer>)
		{
			if (size < 0)
				throw archive_error{ "Negative read size" };
		}

		if (size > std::numeric_limits<unsigned int>::max())
		{
			//if this is being triggered then may need to start using the zip64 algorithm
			auto message = "Read size was too large. Max read size is: "
				+ to_string(std::numeric_limits<unsigned int>::max()) + ", requested size was: "
				+ to_string(size);
			throw archive_error{ message };
		}

		return static_cast<unsigned int>(size);
	}

	iafstream::iafstream(const std::filesystem::path& p)
	{
		open(p);
		return;
	}

	iafstream::iafstream(const std::filesystem::path& archive, const std::filesystem::path& file)
		: iafstream{archive}
	{
		if(is_open())
			open_file(file);
		return;
	}

	iafstream::~iafstream() noexcept
	{
		close();
		return;
	}

	void iafstream::open(const std::filesystem::path& p)
	{
		_archive = open_archive(p);
	}

	void iafstream::close() noexcept
	{
		if (_archive)
		{
			if (_file)
				close_file();
			close_archive(_archive);
		}
	}

	bool iafstream::is_open() const noexcept
	{
		return _archive;
	}

	static void iafstream_open_file_internal(unarchive a)
	{
		if (unzOpenCurrentFile(a) != UNZ_OK)
			throw archive_error{ "error opening file in archive" };
		return;
	}

	void iafstream::open_file(const std::filesystem::path& p)
	{
		assert(_archive);
		if (_file)
			close_file();

		if (!file_exists(_archive, p))
			throw file_not_found{"file not found in archive: " + p.string()};

		//open current file for reading
		iafstream_open_file_internal(_archive);
		_file = true;
		return;
	}

	void iafstream::close_file() noexcept
	{
		assert(_archive);
		if (!_file)
			return;

		if (unzCloseCurrentFile(_archive) == UNZ_CRCERROR)
			LOGWARNING("CRC error reading archive");
		return;
	}

	bool iafstream::is_file_open() const noexcept
	{
		return _file;
	}

	iafstream& iafstream::read(char_t* buffer, std::size_t count)
	{
		assert(buffer);
		if (!_file)
			throw file_not_open{ "iafstream::read tried to read without an open file" };

		//unsigned int type used by ReadCurrentFile
		using z_len_t = unsigned;
		const auto z_len = integer_cast<z_len_t>(count);
		const auto pos = tellg();
		const auto read = unzReadCurrentFile(_archive, buffer, z_len);
		
		if (read == 0) //unz return 0 for eof
			_gcount = tellg() - pos;
		else if (read < 0)
			throw archive_error{ "error decompressing archive" };
		else
			_gcount = integer_cast<std::streamsize>(read);

		return *this;
	}

	std::streamsize iafstream::gcount() const noexcept
	{
		return _gcount;
	}

	iafstream::pos_type iafstream::tellg()
	{
		const auto p = unztell64(_archive);
		if (p == std::numeric_limits<ZPOS64_T>::max())
			throw archive_error{ "iafstream::tellg error, check is_open and is_file_open" };
		return p;
	}

	bool iafstream::eof() const noexcept
	{
		return unzeof(_archive) == 1;
	}

	void iafstream::seekg(pos_type p)
	{
		if (p <= pos_type{})
		{
			seekg({}, std::ios_base::beg);
			return;
		}

		const auto o = p - tellg();
		seekg(o, std::ios_base::cur);
		return;
	}

	constexpr auto buffer_size = std::size_t{ 131072 };
	//constexpr auto buffer_size = std::size_t{ 16384 };

	void iafstream::seekg(off_type o, std::ios_base::seekdir d)
	{
		assert(is_open());
		if (d == std::ios_base::beg)
		{
			close_file();
			iafstream_open_file_internal(_archive);
			_file = true;
			if(o > 0)
				seekg(o, std::ios_base::cur);
		}
		else if (d == std::ios_base::cur)
		{
			if (o == off_type{})
				return;
			else if (o < 0)
			{
				//convert to absolute position
				o = tellg() + o;
				seekg(o);
			}
			else
			{
				//go forward untill we find the target
				auto out_buffer = buffer{ buffer_size };
				while (!eof())
				{
					const auto dist = o - tellg();
					if (dist == 0)
						break;
					else if (dist < buffer_size)
						read(std::data(out_buffer), integer_cast<std::size_t>(dist));
					else
						read(std::data(out_buffer), std::size(out_buffer));
				}
			}
		}
		else if (d == std::ios_base::end)
		{
			unz_file_info64 file_info;
			const auto ret = unzGetCurrentFileInfo64(_archive, &file_info,
				nullptr, 0, 
				nullptr, 0,
				nullptr, 0);

			if (ret != UNZ_OK)
				throw archive_error{ "unexcpected error seeking in archive" };

			const auto size = file_info.uncompressed_size;

			if (o > 0)
				seekg(0, std::ios_base::end);
			else
			{
				const auto target = size + o;
				seekg(target);
			}
		}

		return;
	}

	static bool zlib_inflate_begin(z_stream& stream) noexcept
	{
		stream.zalloc = Z_NULL;
		stream.zfree = Z_NULL;
		stream.opaque = Z_NULL;
		stream.avail_in = 0;
		stream.next_in = Z_NULL;
		return inflateInit(&stream) == Z_OK;
	}

	//tests if the stream is good for izfstream
	static void start_z_stream(izfstream::stream_t& s, z_stream& z)
	{
		zip_header h;
		using char_t = izfstream::stream_t::char_type;
		s.read(reinterpret_cast<char_t*>(std::data(h)), std::size(h));
		if (s.gcount() < static_cast<std::streamsize>(std::size(h))
			|| !probably_compressed(h)
			|| !zlib_inflate_begin(z))
		{
			throw archive_error{ "stream not compressed" };
		}

		s.seekg(izfstream::stream_t::off_type{}, std::ios_base::beg);
		if (!zlib_inflate_begin(z))
			throw archive_error{ "unable to start zlib engine" };
		return;
	}

	constexpr auto if_mode = std::ios_base::in | std::ios_base::binary;

	izfstream::izfstream(const std::filesystem::path& p)
		: _stream{ p, if_mode }
	{
		if (!std::filesystem::exists(p))
			throw files::file_not_found{ "cannot find file: " + p.generic_string() };
		if (!_stream.is_open())
			throw files::file_error{ "cannot open file: " + p.generic_string() };
		start_z_stream(_stream, _zip_stream);
		return;
	}

	izfstream::izfstream(stream_t s)
		: _stream{ std::move(s) }
	{
		if (!_stream.is_open())
			throw files::file_not_open{ "stream passed to izfstream(stream_t) is not open" };
		start_z_stream(_stream, _zip_stream);
		return;
	}

	izfstream::~izfstream() noexcept
	{
		if (is_open())
			inflateEnd(&_zip_stream);
		return;
	}

	void izfstream::open(const std::filesystem::path& p)
	{
		assert(!is_open());
		if (!std::filesystem::exists(p))
			throw files::file_not_found{ "cannot find file: " + p.generic_string() };
		_stream.open(p, if_mode);
		if (!_stream.is_open())
			throw files::file_error{ "cannot open file: " + p.generic_string() };
		start_z_stream(_stream, _zip_stream);
		return;
	}

	void izfstream::close() noexcept
	{
		assert(is_open());
		inflateEnd(&_zip_stream);
		_zip_stream = z_stream{};
		_stream.close();
		_buffer.clear();
		_buffer_pos = std::size_t{};
		_buffer_end = std::size_t{};
		_eof = false;
		return;
	}

	izfstream& izfstream::read(char_t* buffer, std::size_t count)
	{
		if (std::empty(_buffer))
		{
			_buffer.resize(buffer_size);
			using char_type = izfstream::stream_t::char_type;
			_stream.read(reinterpret_cast<char_type*>(std::data(_buffer)), buffer_size);
			_buffer_pos = std::size_t{};
			_buffer_end = static_cast<std::size_t>(_stream.gcount());
		}

		//uint as defined by zlib
		using z_uint = uInt;
		_zip_stream.avail_in = integer_cast<z_uint>(_buffer_end - _buffer_pos);
		_zip_stream.next_in = reinterpret_cast<Bytef*>(std::data(_buffer) + _buffer_pos);
		_zip_stream.avail_out = integer_cast<z_uint>(count);
		_zip_stream.next_out = reinterpret_cast<Bytef*>(buffer);

		const auto ret = ::inflate(&_zip_stream, Z_NO_FLUSH);
		if (ret == Z_STREAM_END)
			_eof = true;
		else if (ret != Z_OK)
			throw archive_error("izfstream failed to inflate during read");

		if (_zip_stream.avail_in == 0)
		{
			_buffer.clear();
			_buffer_pos = std::size_t{};
			_buffer_end = std::size_t{};
		}

		_last_read = static_cast<std::streamsize>(count) -
			static_cast<std::streamsize>(_zip_stream.avail_out);

		// i don't know why this is part of the std streams api
		return *this;
	}

	izfstream::pos_type izfstream::tellg()
	{
		return static_cast<pos_type>(_zip_stream.total_out);
	}

	void izfstream::seekg(pos_type p)
	{
		assert(is_open());
		if (p == pos_type{})
			return;
		
		const auto o = p - tellg();
		seekg(o, std::ios_base::cur);
		return;
	}

	void izfstream::seekg(off_type o, std::ios_base::seekdir d)
	{
		assert(is_open());
		if (d == std::ios_base::beg)
		{
			_stream.seekg(izfstream::stream_t::off_type{}, std::ios_base::beg);
			::inflateReset(&_zip_stream);
			_buffer.clear();
			_buffer_pos = std::size_t{};
			_buffer_end = std::size_t{};
			_eof = false;
			seekg(o, std::ios_base::cur);
		}
		else if (d == std::ios_base::cur)
		{
			if (o == off_type{})
				return;
			else if (o < 0)
			{
				//convert to absolute position
				o = tellg() + o;
				seekg(o);
			}
			else
			{
				//go forward untill we find the target
				auto out_buffer = buffer{ buffer_size };
				while (!eof())
				{
					const auto dist = o - tellg();
					if (dist == 0)
						break;
					else if (dist < buffer_size)
						read(std::data(out_buffer), integer_cast<std::size_t>(dist));
					else
						read(std::data(out_buffer), std::size(out_buffer));
				}
			}
		}
		else if (d == std::ios_base::end)
		{
			if (o == off_type{})
			{
				//seek to end
				auto out_buffer = buffer{ buffer_size };
				while (!eof())
					read(std::data(out_buffer), std::size(out_buffer));
			}
			else if(o > 0)
				seekg(0, std::ios_base::end);
			else
			{
				seekg(0, std::ios_base::end);
				const auto size = tellg();
				const auto target = size + o;
				seekg(target);
			}
		}

		return;
	}

	unarchive open_archive(const fs::path& path)
	{
		if (!fs::exists(path))
			throw files::file_not_found{ "archive not found: " + path.generic_string() };
		//open archive
		unarchive a = unzOpen(path.string().c_str());

		if (!a)
			throw archive_error{ "unable to open archive: " + path.generic_string() };

		return a;
	}

	static void close_archive(unarchive f)
	{
		const auto r = unzClose(f);
		if (r != UNZ_OK)
			throw files::file_error{ "tried to close archive before closing file" };
		return;
	}

	//no sensitivity on windows
	constexpr int case_sensitivity_auto = 0,
		//case sensitivity everywhere
		case_sensitivity_sensitive = 1,
		//no sensitivity on any platform
		case_sensitivity_none = 2;

	bool file_exists(const std::filesystem::path& archive, const std::filesystem::path& path)
	{
		const auto a = open_archive(archive);
		const auto ret = file_exists(a, path );
		close_archive(a);
		return ret;
	}

	static bool file_exists(unarchive a, const std::filesystem::path& path)
	{
		const auto r = unzLocateFile(a, path.string().c_str(), case_sensitivity_sensitive);
		return r == UNZ_OK;
	}

	static std::string_view::iterator::difference_type
		count_separators(const std::filesystem::path& p)
	{
		//constexpr char separator1 = '\\';
		constexpr char separator2 = '/';
		
		const auto s = p.generic_string();

		//auto sep_count = std::count(s.begin(), s.end(), separator1);
		return std::count(s.begin(), s.end(), separator2);// = sep_count;
	}

	std::vector<types::string> list_files_in_archive(const std::filesystem::path& archive, const std::filesystem::path& dir_path, bool recursive)
	{
		std::vector<types::string> output;

		const auto zip = open_archive(archive);

		const auto ret = unzGoToFirstFile(zip);
		if (ret != ZIP_OK)
			throw file_not_found{ "Error finding file in archive: " + archive.generic_string() };

		const auto separator_count = count_separators(dir_path);

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
				if (recursive || count_separators(file_name) == 0)
					output.push_back(file_name);
			}
			else
			{
				//list file if it contains the same number of directory seperators as dir_path or more if recursive
				if (recursive && count_separators(file_name) >= separator_count)
					output.push_back(file_name);
				else if (count_separators(file_name) == separator_count)
				{
					const auto length = dir_path.generic_string().length();
					output.push_back(file_name.substr(length, file_name.length() - length));
				}
			}
		}

		close_archive(zip);

		return output;
	}

	void compress_directory(const std::filesystem::path& path)
	{
		if (!fs::exists(path))
			throw files::file_error{ "Directory not found: " + path.generic_string() };

		if (!fs::is_directory(path))
			throw files::file_error{ "Path is not a directory: " + path.generic_string() };

		const auto archive_name = fs::path{ *--std::end(path) }.replace_extension(archive_ext);
		const auto parent_folder = path / "..";
		const auto final_path = parent_folder / archive_name;
		const auto final_path_str = final_path.generic_string();
		//overwrite if the file already exists
		if (fs::exists(final_path))
		{
			LOGWARNING("Archive already exists overwriting: " + final_path_str);
			std::error_code errorc;
			if (!fs::remove(final_path, errorc))
				throw archive_error{ "Unable to modify existing archive. Reason: " + errorc.message() };
		}

		if (final_path.filename() == ".") // ???
			throw archive_error{ "Tried to open \"./\"." };

		const auto zip = zipOpen(final_path_str.c_str(), APPEND_STATUS_CREATE);

		if (!zip)
			throw archive_error{ "Error creating zip file: " + final_path_str };

		LOG("Created archive: " + final_path_str);

		//for each file in dir
		//create new zip file
		for (auto& f : fs::recursive_directory_iterator(path))
		{
			const auto p = f.path();
			if (fs::is_directory(p))
				continue;

			const auto file_path = p.generic_string();
			const auto directory = path.generic_string();
			if (file_path.find(directory) == types::string::npos)
				throw files::file_error{ "Directory path isn't a subset of the file path, unexpected error" };

			auto file_name = file_path.substr(directory.length(), file_path.length() - directory.length());

			//remove leading /
			file_name.erase(file_name.begin());

			const auto date = fs::last_write_time(p);
			const auto t = date.time_since_epoch();

			//NOTE: if this is firing, then we need to enable zip64 compression below
			if (fs::file_size(p) > 0xffffffff)
				throw archive_error{ "Cannot store file in archive: " + file_name + " file too large" };

			zip_fileinfo file_info{ tm_zip{}, 0, 0, 0 };
			//no const for 'ret' the variable is resued a few times for other results
			auto ret = zipOpenNewFileInZip(zip, file_name.c_str(), &file_info, nullptr, 0, nullptr, 0, nullptr,
				Z_DEFLATED, // 0 = store, Z_DEFLATE = deflate
				Z_DEFAULT_COMPRESSION);

			if (ret != ZIP_OK)
				throw archive_error{ "Error writing file: " + p.filename().generic_string() + " to archive: "
				+ final_path_str };

			using char_buffer = std::vector<char>;

			//NOTE:, the buffer size will have to be double checked if we move to zip64 compression
			std::ifstream in(p.string(), std::ios::binary | std::ios::ate);
			const auto size = in.tellg();

			//if the file is size 0(like many git tag files are)
			//then skip this whole thing and close the file.
			if (size > 0)
			{
				char_buffer buf(static_cast<char_buffer::size_type>(size));
				in.seekg(std::ios::beg);
				in.read(&buf[0], size);

				const auto usize = CheckSizeLimits(static_cast<std::streamoff>(size));
				ret = zipWriteInFileInZip(zip, &buf[0], usize);
				if (ret != ZIP_OK)
					throw archive_error{ "Error writing file: " + p.filename().generic_string() + " to archive: " + final_path_str };
			}

			LOG("Wrote " + file_name + " to archive: " + final_path_str);

			ret = zipCloseFileInZip(zip);
			if (ret != ZIP_OK)
				throw archive_error{ "Error writing file: " + p.filename().generic_string() + " to archive: " + final_path_str };
		}

		const auto ret = zipClose(zip, nullptr);
		if (ret != ZIP_OK)
			throw archive_error{ "Error closing file: " + final_path_str };

		LOG("Completed creating archive: " + final_path_str);
	}

	void uncompress_archive(const std::filesystem::path& fs_path)
	{
		//confirm is archive
		if (!fs::exists(fs_path))
			throw files::file_not_found{ "Cannot open archive, file not found: " + fs_path.generic_string() };

		//create directory if absent
		const auto root_dir = fs_path.parent_path() / fs_path.stem();

		const auto archive = open_archive(fs_path);

		//NOTE: no const for ret, variable is resued later
		auto ret = unzGoToFirstFile(archive);
		if (ret != ZIP_OK)
			throw archive_error{ "Error finding file in archive: " + fs_path.generic_string() };

		LOG("Uncompressing archive: " + fs_path.generic_string());

		//for each file in directory
		do {
			unz_file_info info;
			unzGetCurrentFileInfo(archive, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			using char_buffer = std::vector<char>;
			char_buffer name(info.size_filename);

			ret = unzGetCurrentFileInfo(archive, &info, &name[0], info.size_filename, nullptr, 0, nullptr, 0);
			if (ret != ZIP_OK)
				throw archive_error{ "Error reading file info from archive" };

			const types::string filename_str(name.begin(), name.end());
			const auto filename = fs::path{ filename_str };

			//create directory if it dosn't already exist
			fs::create_directories(root_dir / filename.parent_path()); //only creates missing directories

			//open file
			std::ofstream file(root_dir / filename, std::ios::binary | std::ios::trunc);

			if (!file.is_open())
				throw files::file_error{ "Failed to create or open file: " + filename.generic_string() };

			//write data to file
			const auto usize = CheckSizeLimits(info.uncompressed_size);
			using char_buffer = std::vector<char>;
			char_buffer buff(static_cast<buffer::size_type>(usize));

			ret = unzOpenCurrentFile(archive);
			if (ret != ZIP_OK)
				throw archive_error{ "Failed to open file in archive: " + (fs_path / filename).generic_string() };

			types::uint32 total_written = 0;

			LOG("Uncompressing file: " + filename.generic_string() + " from archive: " + fs_path.generic_string());

			//write the data to actual files
			while (total_written < usize)
			{
				const auto amount = unzReadCurrentFile(archive, &buff[0], usize);
				file.write(buff.data(), amount);
				total_written += amount;
			}

			ret = unzCloseCurrentFile(archive);
			if (ret != ZIP_OK)
				throw archive_error{ "Failed to close file in archive: " + (fs_path / filename).generic_string() };
		} while (unzGoToNextFile(archive) != UNZ_END_OF_LIST_OF_FILE);

		close_archive(archive);

		LOG("Finished uncompressing archive: " + fs_path.generic_string());
	}

	namespace header
	{
		constexpr std::byte first{ 0x78 };
		constexpr std::array<std::byte, 3> others{
			static_cast<std::byte>(0x01),
			static_cast<std::byte>(0x9C),
			static_cast<std::byte>(0xDA)
		};
	}

	bool probably_compressed(zip_header header)
	{
		return header[0] == header::first &&
			std::any_of(std::begin(header::others), std::end(header::others), [second = header[1]](auto&& other)
		{
			return second == other;
		});
	}

	bool probably_compressed(const buffer& stream)
	{
		if (stream.size() < 2)
			return false;

		return probably_compressed(std::array{ stream[0], stream[1] });
	}

	buffer deflate(buffer stream)
	{
		if (!*hades::console::get_bool(cvars::file_deflate, true))
			return stream;

		z_stream deflate_stream;
		deflate_stream.zalloc = Z_NULL;
		deflate_stream.zfree = Z_NULL;
		deflate_stream.opaque = Z_NULL;

		//uint as defined by zlib
		using z_uint = uInt;

		deflate_stream.avail_in =
			integer_cast<z_uint>(std::size(stream)); // size of input, string + terminator
		deflate_stream.next_in = reinterpret_cast<unsigned char*>(stream.data()); // input char array

		// the actual compression work.
		auto ret = deflateInit(&deflate_stream, Z_BEST_COMPRESSION);
		if (ret != Z_OK)
			throw archive_error{ "failed to initialise zlib deflate" };

		//unsigned long as defined by zlib;
		using z_ulong = uLong;

		//get the size
		const auto size = deflateBound(&deflate_stream, integer_cast<z_ulong>(std::size(stream)));

		buffer out{ size };
		deflate_stream.avail_out = integer_cast<z_uint>(std::size(out)); // size of output
		deflate_stream.next_out = reinterpret_cast<unsigned char*>(out.data()); // output char array

		ret = deflate(&deflate_stream, Z_FINISH);
		if (ret != Z_STREAM_END)
			throw archive_error{ "failed to deflate" };
		assert(ret == Z_STREAM_END);
		ret = deflateEnd(&deflate_stream);
		if (ret != Z_OK)
			throw archive_error{ "failed to finalise zlib deflate" };

		return { out.data(), reinterpret_cast<std::byte*>(deflate_stream.next_out) };
	}

	buffer inflate(buffer stream)
	{
		z_stream infstream;
		infstream.zalloc = Z_NULL;
		infstream.zfree = Z_NULL;
		infstream.opaque = Z_NULL;

		//uint as defined by zlib
		using z_uint = uInt;

		// setup "b" as the input and "c" as the compressed output
		infstream.avail_in =
			integer_cast<z_uint>(std::size(stream)); // size of input
		infstream.next_in = reinterpret_cast<unsigned char*>(stream.data()); // input char array
		//infstream.avail_out = (uInt)sizeof(c); // size of output
		//infstream.next_out = (Bytef *)c; // output char array

		 // the actual DE-compression work.
		auto ret = inflateInit(&infstream);
		if (ret != Z_OK)
			throw archive_error{ "failed to initialise zlib inflate" };

		buffer out{};
		out.reserve(std::size(stream));

		bool cont = true;
		while (cont)
		{
			buffer buf{ stream.size() };
			infstream.avail_out = integer_cast<z_uint>(std::size(buf));
			infstream.next_out = reinterpret_cast<unsigned char*>(buf.data());
			ret = inflate(&infstream, Z_NO_FLUSH);
			if (ret == Z_STREAM_END)
				cont = false;
			else if (ret != Z_OK)
				throw archive_error{ "failed to inflate" };
			out.reserve(out.size() + buf.size());
			std::copy(buf.data(), reinterpret_cast<std::byte*>(infstream.next_out), std::back_inserter(out));

			if (infstream.avail_out != 0)
				cont = false;
		}

		ret = inflateEnd(&infstream);
		if (ret != Z_OK)
			throw archive_error{ "failed to finalise zlib inflate" };

		return out;
	}
}
