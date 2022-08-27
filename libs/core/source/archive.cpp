#include "hades/archive.hpp"

#include <cassert>
#include <filesystem>

#include "zlib.h"
#include "zip.h"
#include "unzip.h"
#undef zlib_version

#include "hades/console_variables.hpp"
#include "hades/logging.hpp"
#include "hades/properties.hpp"
#include "hades/utility.hpp"

namespace fs = std::filesystem;

using namespace std::string_literals;
using namespace std::string_view_literals;

//this is the default archive extension
//used when created archives, but not when searching for them
constexpr auto archive_ext = ".zip"sv;

namespace hades::zip
{
	std::string_view zlib_version() noexcept
	{
		constexpr auto version = std::string_view{ ZLIB_VERSION };
		return version;
	}

	template<typename Integer>
	static unsigned int CheckSizeLimits(Integer size)
	{
		if constexpr (std::is_signed_v<Integer>)
		{
			if (size < 0)
				throw archive_error{ "Negative read size"s };
		}

		if (size > std::numeric_limits<unsigned int>::max())
		{
			//if this is being triggered then may need to start using the zip64 algorithm
			auto message = "Read size was too large. Max read size is: "s
				+ to_string(std::numeric_limits<unsigned int>::max()) + ", requested size was: "s
				+ to_string(size);
			throw archive_error{ message };
		}

		return integer_cast<unsigned int>(size);
	}

	iafstream::iafstream() noexcept
		: std::istream{ &_stream }
	{}

	iafstream::iafstream(const std::filesystem::path& p)
		: std::istream{ &_stream }
	{
		_stream.open(p);
		return;
	}

	iafstream::iafstream(const std::filesystem::path& a, const std::filesystem::path& f)
		: std::istream{ &_stream }
	{
		if (!_stream.open(a))
			throw archive_error{ "Failed to open archive: "s + a.generic_string() };
		_stream.open_file(f);
		return;
	}

	iafstream::iafstream(iafstream&& rhs) noexcept
		: std::istream{ &_stream }, _stream{ std::move(rhs._stream) }
	{
		rhs._stream = {};
		return;
	}

	iafstream& iafstream::operator=(iafstream&& rhs) noexcept
	{
		std::swap(_stream, rhs._stream);
		return *this;
	}

	void iafstream::open(const std::filesystem::path& p)
	{
		_stream.open(p);
		return;
	}

	void iafstream::open_file(const std::filesystem::path& p)
	{
		try
		{
			_stream.open_file(p);
			clear();
		}
		catch (...)
		{
			setstate(failbit);
			throw;
		}
		return;
	}

	void iafstream::close_file()
	{
		try
		{
			_stream.close_file();
			return;
		}
		catch (...)
		{
			setstate(failbit);
			throw;
		}
	}

	void oafstream::open(const std::filesystem::path& p)
	{
		_archive = create_archive(p);
		return;
	}

	void oafstream::close() noexcept
	{
		close_archive(_archive);
		return;
	}

	bool oafstream::is_open() const noexcept
	{
		return _archive.handle;
	}

	void oafstream::open_file(const std::filesystem::path& path)
	{
		auto path_str = path.generic_string();

		zip_fileinfo file_info{ tm_zip{}, 0, 0, 0 };
		//no const for 'ret' the variable is resued a few times for other results
		auto ret = zipOpenNewFileInZip(_archive.handle, path_str.c_str(), &file_info, nullptr, 0, nullptr, 0, nullptr,
			Z_DEFLATED, // 0 = store, Z_DEFLATE = deflate
			Z_DEFAULT_COMPRESSION);

		if (ret != ZIP_OK)
			throw archive_error{ "Error creating file: "s + path_str + " in archive: "s
			+ _archive.path };

		_file = std::move(path_str);

		return;
	}

	void oafstream::close_file() noexcept
	{
		assert(!empty(_file));
		const auto ret = zipCloseFileInZip(_archive.handle);
		if (ret != ZIP_OK)
			log_error( "Error writing file: "s + _file + " to archive: "s + _archive.path );

		_file.clear();
		return;
	}

	oafstream& oafstream::write(const char_t* buffer, std::streamsize count)
	{
		assert(buffer);
		const auto ret = zipWriteInFileInZip(_archive.handle, buffer, integer_cast<unsigned int>(count));
		if (ret != ZIP_OK)
			throw archive_error{ "Error writing file: "s + _file + " to archive: "s + _archive.path };

		return *this;
	}

	// in file mode
	constexpr auto if_mode = std::ios_base::binary;


	izfstream::izfstream() noexcept
		: _stream{ std::filebuf{} },
		std::istream{ nullptr }
	{}

	izfstream::izfstream(const std::filesystem::path& p) 
		: std::istream{ nullptr }
	{
		_stream = in_compressed_filebuf{ p };
		rdbuf(&std::get<in_compressed_filebuf>(_stream));
		if (!is_open())
		{
			// try normal file opening
			auto p_str = p.string();
			auto file = std::fopen(p_str.c_str(), "rb");
			_stream = std::filebuf{ file };
			rdbuf(&std::get<std::filebuf>(_stream));

			if (!is_open())
			{
				setstate(badbit);
				if (!std::filesystem::exists(p))
					throw files::file_not_found{ "cannot find file: "s + p.generic_string() };
				throw files::file_error{ "cannot open file: "s + p.generic_string() };
			}
		}

		clear();
		return;
	}

	/*izfstream::izfstream(stream_t s)
		: _stream{ std::move(s) }, _zip_stream{ new z_stream, delete_z_stream }
	{
		if (!_stream.is_open())
			throw files::file_not_open{ "stream passed to izfstream(stream_t) is not open"s };
		start_z_stream(_stream, _zip_stream->stream);
		return;
	}*/

	izfstream::izfstream(izfstream&& rhs) noexcept
		: std::istream{ std::move(rhs) }
	{
		_stream = std::move(rhs._stream);
		rhs._stream = {};
		std::streambuf* ptr = {};
		std::visit([&ptr](auto& stream) {
			ptr = &stream;
			return;
			}, _stream);

		rdbuf(ptr);
		rhs.rdbuf(nullptr);
		return;
	}


	izfstream& izfstream::operator=(izfstream&& rhs) noexcept
	{
		std::istream::operator=(std::move(rhs));
		_stream = std::move(rhs._stream);
		rhs._stream = {};
		std::streambuf* ptr = {};
		std::visit([&ptr](auto& stream) {
			ptr = &stream;
			return;
			}, _stream);

		rdbuf(ptr);
		rhs.rdbuf(nullptr);
		return *this;
	}

	void izfstream::open(const std::filesystem::path& p)
	{
		assert(!is_open());
		_stream = in_compressed_filebuf{ p };
		rdbuf(&std::get<in_compressed_filebuf>(_stream));

		if (!is_open())
		{
			auto p_str = p.string();
			auto file = std::fopen(p_str.c_str(), "rb");
			_stream = std::filebuf{ file };
			rdbuf(&std::get<std::filebuf>(_stream));
		}

		if (!is_open())
		{
			setstate(badbit);
			if (!std::filesystem::exists(p))
				throw files::file_not_found{ "cannot find file: "s + p.generic_string() };
			throw files::file_error{ "cannot open file: "s + p.generic_string() };
		}

		clear();
		return;
	}

	void izfstream::close() noexcept
	{
		assert(is_open());
		std::visit([](auto&& stream) {
			stream.close();
			return;
			}, _stream);
		return;
	}

	ozfstream::ozfstream() noexcept
		: std::ostream{ &_streambuf }
	{}

	ozfstream::ozfstream(const std::filesystem::path& p)
		: std::ostream{ nullptr }
	{
		open(p);
		return;
	}

	ozfstream::ozfstream(ozfstream&& rhs) noexcept
		: _streambuf{ std::move(rhs._streambuf) },
		std::ostream{ nullptr }
	{
		rdbuf(&_streambuf);
		rhs.rdbuf(nullptr);
		rhs._streambuf = {};
		return;
	}

	ozfstream& ozfstream::operator=(ozfstream&& rhs) noexcept
	{
		_streambuf = std::move(rhs._streambuf); 
		rdbuf(&_streambuf);
		rhs.rdbuf(nullptr);
		rhs._streambuf = {};
		return *this;
	}

	void ozfstream::open(const std::filesystem::path& p)
	{
		assert(!is_open());
		_streambuf.open(p);
		return;
	}

	void ozfstream::close() noexcept
	{
		assert(is_open());
		_streambuf.close();
		return;
	}

	bool ozfstream::is_open() noexcept
	{
		return _streambuf.is_open();
	}

	bool file_exists(const std::filesystem::path& archive, const std::filesystem::path& path)
	{
		auto a = open_archive(archive);
		const auto ret = file_exists(a, path );
		close_archive(a);
		return ret;
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

		auto zip = open_archive(archive);

		const auto ret = unzGoToFirstFile(zip.handle);
		if (ret != ZIP_OK)
			throw file_not_found{ "Error finding file in archive: " + archive.generic_string() };

		const auto separator_count = count_separators(dir_path);

		//while there's more files
		while (unzGoToNextFile(zip.handle) != UNZ_END_OF_LIST_OF_FILE)
		{
			unz_file_info info;
			unzGetCurrentFileInfo(zip.handle, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			using char_buffer = std::vector<char>;
			char_buffer name(info.size_filename);

			unzGetCurrentFileInfo(zip.handle, &info, &name[0], info.size_filename, nullptr, 0, nullptr, 0);

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
			throw files::file_error{ "Directory not found: "s + path.generic_string() };

		if (!fs::is_directory(path))
			throw files::file_error{ "Path is not a directory: "s + path.generic_string() };

		const auto archive_name = fs::path{ *--std::end(path) }.replace_extension(archive_ext);
		const auto parent_folder = path.parent_path();
		const auto final_path = parent_folder / archive_name;

		if (final_path.filename() == "."s)
			throw archive_error{ "Tried to open \"./\"."s };

		auto zip_stream = oafstream{ final_path };
		assert(zip_stream.is_open());
		log("Opened archive for writing: " + final_path.generic_string());

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

			//NOTE: if this is firing, then we need to enable zip64 compression below
			if (fs::file_size(p) > 0xffffffff)
				throw archive_error{ "Cannot store file in archive: "s + file_name + " file too large"s };

			assert(zip_stream.is_file_open() == false);
			zip_stream.open_file(file_name);
			
			auto in_stream = std::ifstream{ p, std::ios::binary };		
			auto buf = std::array<std::byte, default_buffer_size>{};
			// if buffer size needs to be increased, then we probably need to upgrade 
			// to the zip64 funcs
			static_assert(default_buffer_size < std::numeric_limits<unsigned int>::max(),
				"buffer size must be smaller than the read limit for zip32");
			do
			{
				constexpr auto read_size = integer_cast<std::streamsize>(default_buffer_size);
				in_stream.read(reinterpret_cast<char*>(buf.data()), read_size);
				zip_stream.write(buf.data(), in_stream.gcount());
			} while (in_stream.gcount() == default_buffer_size);

			log_debug("Wrote "s + file_name + " to archive: "s + final_path.generic_string());
			zip_stream.close_file();
		}

		log("Finished writing archive: "s + final_path.string());
	}

	void uncompress_archive(const std::filesystem::path& fs_path)
	{
		//confirm is archive
		if (!fs::exists(fs_path))
			throw files::file_not_found{ "Cannot open archive, file not found: " + fs_path.generic_string() };

		//create directory if absent
		const auto root_dir = fs_path.parent_path() / fs_path.stem();
		auto archive = open_archive(fs_path);

		const auto finally = make_finally([&archive]() noexcept {
			close_archive(archive);
			return;
			});

		//NOTE: no const for ret, variable is resued later
		auto ret = unzGoToFirstFile(archive.handle);
		if (ret != ZIP_OK)
			throw archive_error{ "Error finding file in archive.handle: " + fs_path.generic_string() };

		LOG("Uncompressing archive.handle: " + fs_path.generic_string());

		//for each file in directory
		do {
			unz_file_info info;
			unzGetCurrentFileInfo(archive.handle, &info, nullptr, 0, nullptr, 0, nullptr, 0);

			using char_buffer = std::vector<char>;
			char_buffer name(info.size_filename);

			ret = unzGetCurrentFileInfo(archive.handle, &info, &name[0], info.size_filename, nullptr, 0, nullptr, 0);
			if (ret != ZIP_OK)
				throw archive_error{ "Error reading file info from archive.handle" };

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
			char_buffer buff(integer_cast<buffer::size_type>(usize));

			ret = unzOpenCurrentFile(archive.handle);
			if (ret != ZIP_OK)
				throw archive_error{ "Failed to open file in archive.handle: " + (fs_path / filename).generic_string() };

			types::uint32 total_written = 0;

			LOG("Uncompressing file: " + filename.generic_string() + " from archive.handle: " + fs_path.generic_string());

			//write the data to actual files
			while (total_written < usize)
			{
				const auto amount = unzReadCurrentFile(archive.handle, &buff[0], usize);
				file.write(buff.data(), amount);
				total_written += amount;
			}

			ret = unzCloseCurrentFile(archive.handle);
			if (ret != ZIP_OK)
				throw archive_error{ "Failed to close file in archive.handle: " + (fs_path / filename).generic_string() };
		} while (unzGoToNextFile(archive.handle) != UNZ_END_OF_LIST_OF_FILE);

		LOG("Finished uncompressing archive: " + fs_path.generic_string());
	}

	bool probably_compressed(const buffer& stream) noexcept
	{
		if (stream.size() < 2)
			return false;

		return probably_compressed(std::array{ stream[0], stream[1] });
	}

	buffer deflate(buffer stream)
	{
		if (!*hades::console::get_bool(cvars::file_deflate, cvars::default_value::file_deflate))
			return stream;

		::z_stream deflate_stream;
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
		::z_stream infstream;
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
			out.insert(std::end(out), buf.data(), reinterpret_cast<std::byte*>(infstream.next_out));

			if (infstream.avail_out != 0)
				cont = false;
		}

		ret = inflateEnd(&infstream);
		if (ret != Z_OK)
			throw archive_error{ "failed to finalise zlib inflate" };

		return out;
	}
}
