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
	archive_exception::archive_exception(const char* what, error_code code)
		: std::runtime_error(what), _code(code)
	{}

	//open a close unzip archives
	unarchive open_archive(std::filesystem::path);
	void close_archive(unarchive f);

	archive_stream::archive_stream(std::string_view archive) : _fileOpen(false), _archive(open_archive(archive))
	{}

	archive_stream::archive_stream(archive_stream&& rhs) : _fileOpen(rhs._fileOpen),
		_archive(rhs._archive), _fileName(rhs._fileName)
	{
		rhs._archive = nullptr;
		rhs._fileOpen = false;
	}

	archive_stream& archive_stream::operator=(archive_stream&& rhs)
	{
		_close();

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
		_close();
	}

	static bool file_exists(unarchive, fs::path);

	bool archive_stream::open(std::string_view filename)
	{
		if (_fileOpen)
			throw archive_exception("This stream already has a file open", archive_exception::error_code::FILE_ALREADY_OPEN);

		if (!file_exists(_archive, filename))
			return false;

		//open current file for reading
		const auto r = unzOpenCurrentFile(_archive);
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

	template<typename Integer>
	unsigned int CheckSizeLimits(Integer size)
	{
		if constexpr (std::is_signed_v<Integer>)
		{
			if (size < 0)
				throw archive_exception("Negative read size", archive_exception::error_code::FILE_READ);
		}

		if (size > std::numeric_limits<unsigned int>::max())
		{
			//if this is being triggered then may need to start using the zip64 algorithm
			auto message = "Read size was too large. Max read size is: "
				+ to_string(std::numeric_limits<unsigned int>::max()) + ", requested size was: "
				+ to_string(size);
			throw archive_exception(message.c_str(), archive_exception::error_code::FILE_READ);
		}

		return static_cast<unsigned int>(size);
	}

	sf::Int64 archive_stream::read(void* data, sf::Int64 size)
	{
		if (data == nullptr)
			throw std::invalid_argument("Must pass a valid buffer to archive_stream::read");

		if (!_fileOpen)
			throw archive_exception("Tried to read without an open file", archive_exception::error_code::FILE_NOT_OPEN);

		if (size == 0)
		{
			LOGWARNING("Tried to read 0 bytes from file; archive_stream::read");
			return 0;
		}

		//unsigned int type used by zlib
		const auto usize = CheckSizeLimits(size);

		buffer buff(static_cast<buffer::size_type>(usize));

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

		if (!open(_fileName))
			throw archive_exception(("Failed to open file in archive: " + _fileName).c_str(), archive_exception::error_code::FILE_OPEN);

		auto upos = CheckSizeLimits(position);
		buffer buff(static_cast<buffer::size_type>(upos));

		sf::Int64 total = 0;

		while (total < position)
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

		const auto r = unzGetCurrentFileInfo64(_archive, &info, nullptr, 0, nullptr, 0, nullptr, 0);

		if (r != UNZ_OK)
			throw archive_exception("Unable to get file into.", archive_exception::error_code::FILE_READ);

		return info.uncompressed_size;
	}

	void archive_stream::_close()
	{
		if (_fileOpen)
			unzCloseCurrentFile(_archive); // returns error code if CRC check failed(we don't care at this point)

		if (_archive)
			unzClose(_archive); // returns UNZ_OK if everything was fine(what could go wrong that we would bother to report?)
	}

	iafstream::iafstream(std::filesystem::path p)
	{
		open(p);
		return;
	}

	iafstream::iafstream(std::filesystem::path archive, std::filesystem::path file)
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

	void iafstream::open(std::filesystem::path p)
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

	void iafstream::open_file(std::filesystem::path p)
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
		if (s.gcount() < std::size(h)
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

	izfstream::izfstream(std::filesystem::path p)
		: _stream{ p, if_mode }
	{
		if (!std::filesystem::exists(p))
			throw archive_exception{ "cannot find file", archive_exception::error_code::FILE_NOT_FOUND };
		if (!_stream.is_open())
			throw archive_exception{ "cannot open file", archive_exception::error_code::FILE_OPEN };
		start_z_stream(_stream, _zip_stream);
		return;
	}

	izfstream::izfstream(stream_t s)
		: _stream{ std::move(s) }
	{
		if (!_stream.is_open())
			throw archive_exception{ "stream not open", archive_exception::error_code::FILE_NOT_OPEN };
		start_z_stream(_stream, _zip_stream);
		return;
	}

	izfstream::~izfstream() noexcept
	{
		if (is_open())
			inflateEnd(&_zip_stream);
		return;
	}

	void izfstream::open(std::filesystem::path p)
	{
		assert(!is_open());
		if (!std::filesystem::exists(p))
			throw archive_exception{ "cannot find file", archive_exception::error_code::FILE_NOT_FOUND };
		_stream.open(p, if_mode);
		if (!_stream.is_open())
			throw archive_exception{ "cannot open file", archive_exception::error_code::FILE_OPEN };
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
			throw archive_exception("failed to inflate");

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

	unarchive open_archive(fs::path path)
	{
		if (!fs::exists(path))
			throw files::file_not_found{ "archive not found: " + path.string() };
		//open archive
		unarchive a = unzOpen(path.string().c_str());

		if (!a)
			throw archive_exception(("unable to open archive: " + path.string()).c_str(), archive_exception::error_code::FILE_OPEN);

		return a;
	}

	void close_archive(unarchive f)
	{
		const auto r = unzClose(f);
		if (r != UNZ_OK)
			throw files::file_error{ "tried to close archive before closing file" };
		return;
	}

	buffer read_file_from_archive(std::string_view archive, std::string_view path)
	{
		auto stream = stream_file_from_archive(archive, path);

		const auto size = stream.getSize();
		const auto usize = CheckSizeLimits(size);

		buffer buff(static_cast<buffer::size_type>(usize));
		stream.read(&buff[0], size);

		return buff;
	}

	types::string read_text_from_archive(std::string_view archive, std::string_view path)
	{
		const auto buf = read_file_from_archive(archive, path);

		types::string out;

		std::transform(std::begin(buf), std::end(buf), std::back_inserter(out), [](hades::buffer::value_type v) {
			return static_cast<char>(v);
			});

		return out;
	}

	archive_stream stream_file_from_archive(std::string_view archive, std::string_view path)
	{
		archive_stream str(archive);
		str.open(path);
		return str;
	}

	//no sensitivity on windows
	constexpr int case_sensitivity_auto = 0,
		//case sensitivity everywhere
		case_sensitivity_sensitive = 1,
		//no sensitivity on any platform
		case_sensitivity_none = 2;

	bool file_exists(std::filesystem::path archive, std::filesystem::path path)
	{
		const auto a = open_archive(archive);
		const auto ret = file_exists(a, path );
		close_archive(a);
		return ret;
	}

	static bool file_exists(unarchive a, fs::path path)
	{
		const auto r = unzLocateFile(a, path.string().c_str(), case_sensitivity_sensitive);
		return r == UNZ_OK;
	}

	static std::string_view::iterator::difference_type
		count_separators(std::string_view s)
	{
		constexpr char separator1 = '\\';
		constexpr char separator2 = '/';

		auto sep_count = std::count(s.begin(), s.end(), separator1);
		return sep_count + std::count(s.begin(), s.end(), separator2);
	}

	std::vector<types::string> list_files_in_archive(std::string_view archive, std::string_view dir_path, bool recursive)
	{
		std::vector<types::string> output;

		const auto zip = open_archive(archive);

		const auto ret = unzGoToFirstFile(zip);
		if (ret != ZIP_OK)
			throw archive_exception(("Error finding file in archive: " + to_string(archive)).c_str(), archive_exception::error_code::FILE_OPEN);

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
					output.push_back(file_name.substr(dir_path.length(), file_name.length() - dir_path.length()));
			}
		}

		close_archive(zip);

		return output;
	}

	void compress_directory(std::string_view path_view)
	{
		const types::string path{ path_view };

		if (!fs::is_directory(path))
			throw archive_exception(("Path is not a directory: " + path).c_str(), archive_exception::error_code::FILE_NOT_FOUND);

		if (!fs::exists(path))
			throw archive_exception(("Directory not found: " + path).c_str(), archive_exception::error_code::FILE_NOT_FOUND);

		//overwrite if the file already exists
		if (fs::exists(path + ".zip"))
		{
			LOGWARNING("Archive already exists overwriting: " + path + ".zip");
			std::error_code errorc;
			if (!fs::remove(path + ".zip", errorc))
				archive_exception(("Unable to modify existing archive. Reason: " + errorc.message()).c_str(), archive_exception::error_code::FILE_WRITE);
		}

		types::string new_zip_path;

		fs::path fs_path(path);
		const auto root = fs_path.parent_path();
		const auto name = --fs_path.end();

		if (*name == ".")
			throw archive_exception("Tried to open \"./\".", archive_exception::error_code::FILE_OPEN);

		const auto zip = zipOpen((root.generic_string() + "/" + name->generic_string() + archive_ext).c_str(), APPEND_STATUS_CREATE);

		if (!zip)
			throw archive_exception(("Error creating zip file: " + name->generic_string()).c_str(), archive_exception::error_code::FILE_WRITE);

		LOG("Created archive: " + name->generic_string());

		//for each file in dir
		//create new zip file
		for (auto& f : fs::recursive_directory_iterator(fs_path))
		{
			const auto p = f.path();
			if (fs::is_directory(p))
				continue;

			const auto file_path = p.string();
			const auto directory = fs_path.string();
			if (file_path.find(directory) == types::string::npos)
				throw archive_exception("Directory path isn't a subset of the file path, unexpected error", archive_exception::error_code::FILE_READ);

			auto file_name = file_path.substr(directory.length(), file_path.length() - directory.length());

			//remove leading /
			file_name.erase(file_name.begin());

			const auto date = fs::last_write_time(p);
			const auto t = date.time_since_epoch();

			//NOTE: if this is firing, then we need to enable zip64 compression below
			if (fs::file_size(p) > 0xffffffff)
				throw archive_exception(("Cannot store file in archive: " + file_name + " file too large").c_str(), archive_exception::error_code::FILE_WRITE);

			zip_fileinfo file_info{ tm_zip{}, 0, 0, 0 };
			//no const for 'ret' the variable is resued a few times for other results
			auto ret = zipOpenNewFileInZip(zip, file_name.c_str(), &file_info, nullptr, 0, nullptr, 0, nullptr,
				Z_DEFLATED, // 0 = store, Z_DEFLATE = deflate
				Z_DEFAULT_COMPRESSION);

			if (ret != ZIP_OK)
				throw archive_exception(("Error writing file: " + p.filename().generic_string() + " to archive: " + name->generic_string()).c_str(), archive_exception::error_code::FILE_WRITE);

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
					throw archive_exception(("Error writing file: " + p.filename().generic_string() + " to archive: " + name->generic_string()).c_str(), archive_exception::error_code::FILE_OPEN);
			}

			LOG("Wrote " + file_name + " to archive: " + name->generic_string());

			ret = zipCloseFileInZip(zip);
			if (ret != ZIP_OK)
				throw archive_exception(("Error writing file: " + p.filename().generic_string() + " to archive: " + name->generic_string()).c_str(), archive_exception::error_code::FILE_OPEN);
		}

		const auto ret = zipClose(zip, nullptr);
		if (ret != ZIP_OK)
			throw archive_exception(("Error closing file: " + name->generic_string()).c_str(), archive_exception::error_code::FILE_CLOSE);

		LOG("Completed creating archive: " + name->generic_string());
	}

	void uncompress_archive(std::string_view path_view)
	{
		const types::string path{ path_view };

		//confirm is archive
		fs::path fs_path(path);
		if (!fs::exists(fs_path))
			throw archive_exception(("Cannot open archive, file not found: " + path).c_str(), archive_exception::error_code::FILE_NOT_FOUND);

		//create directory if absent
		const auto root_dir = fs_path.parent_path() / fs_path.stem();

		const auto archive = open_archive(fs::path{ path });

		//NOTE: no const for ret, variable is resued later
		auto ret = unzGoToFirstFile(archive);
		if (ret != ZIP_OK)
			throw archive_exception(("Error finding file in archive: " + path).c_str(), archive_exception::error_code::FILE_OPEN);

		LOG("Uncompressing archive: " + path);

		//for each file in directory
		do {
			unz_file_info info;
			unzGetCurrentFileInfo(archive, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			using char_buffer = std::vector<char>;
			char_buffer name(info.size_filename);

			ret = unzGetCurrentFileInfo(archive, &info, &name[0], info.size_filename, nullptr, 0, nullptr, 0);
			if (ret != ZIP_OK)
				throw archive_exception("Error reading file info from archive", archive_exception::error_code::FILE_OPEN);

			const types::string filename(name.begin(), name.end());

			const fs::path parent_dir(filename);
			//create directory if it dosn't already exist
			fs::create_directories(root_dir / parent_dir.parent_path()); //only creates missing directories

			//open file
			std::ofstream file(root_dir.string() + "/" + filename, std::ios::binary | std::ios::trunc);

			if (!file.is_open())
				throw archive_exception(("Failed to create or open file: " + filename).c_str(), archive_exception::error_code::FILE_WRITE);

			//write data to file
			const auto usize = CheckSizeLimits(info.uncompressed_size);
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
				const auto amount = unzReadCurrentFile(archive, &buff[0], usize);
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
			throw archive_exception("failed to initialise zlib deflate");

		//unsigned long as defined by zlib;
		using z_ulong = uLong;

		//get the size
		const auto size = deflateBound(&deflate_stream, integer_cast<z_ulong>(std::size(stream)));

		buffer out{ size };
		deflate_stream.avail_out = integer_cast<z_uint>(std::size(out)); // size of output
		deflate_stream.next_out = reinterpret_cast<unsigned char*>(out.data()); // output char array

		ret = deflate(&deflate_stream, Z_FINISH);
		if (ret != Z_STREAM_END)
			throw archive_exception("failed to deflate");
		assert(ret == Z_STREAM_END);
		ret = deflateEnd(&deflate_stream);
		if (ret != Z_OK)
			throw archive_exception("failed to finalise zlib deflate");

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
			throw archive_exception("failed to initialise zlib inflate");

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
				throw archive_exception("failed to inflate");
			out.reserve(out.size() + buf.size());
			std::copy(buf.data(), reinterpret_cast<std::byte*>(infstream.next_out), std::back_inserter(out));

			if (infstream.avail_out != 0)
				cont = false;
		}

		ret = inflateEnd(&infstream);
		if (ret != Z_OK)
			throw archive_exception("failed to finalise zlib inflate");

		return out;
	}
}
