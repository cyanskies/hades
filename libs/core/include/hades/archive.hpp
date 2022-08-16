#ifndef HADES_ARCHIVE_HPP
#define HADES_ARCHIVE_HPP

#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "hades/exceptions.hpp"
#include "hades/string.hpp"

namespace hades
{
	//buffer of bytes
	using buffer = std::vector<std::byte>;
	//NOTE: .net recently moved from 4kb default to 8kb
	constexpr auto default_buffer_size = std::size_t{ 4096 }; //4kb buffer
}

namespace hades::files
{
	class file_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	class file_not_found : public file_error
	{
	public:
		using file_error::file_error;
	};

	class file_not_open : public file_error
	{
	public:
		using file_error::file_error;
	};
}

namespace hades::zip
{
	struct unarchive
	{
		string path;
		void* handle = {};
	};

	struct toarchive
	{
		string path;
		void* handle = {};
	};

	struct z_stream;

	class archive_error : public files::file_error
	{
	public:
		using file_error::file_error;
	};

	//TODO: rename archive member not found
	class file_not_found : public archive_error
	{
	public:
		using archive_error::archive_error;
	};

	class file_not_open : public archive_error
	{
	public:
		using archive_error::archive_error;
	};

	std::string_view zlib_version() noexcept;

	//in archive file stream
	class iafstream
	{
	public:
		using char_t = std::byte;
		using stream_t = std::ifstream;
		using pos_type = stream_t::traits_type::pos_type;
		using off_type = stream_t::traits_type::off_type;

		iafstream() noexcept = default;

		// throws files::file_not_found and archive_error
		explicit iafstream(const std::filesystem::path& p)
		{
			open(p);
			return;
		}

		iafstream(const std::filesystem::path& archive, const std::filesystem::path& file);

		iafstream(const iafstream&) = delete;
		iafstream& operator=(const iafstream&) = delete;

		iafstream(iafstream&&) noexcept = default;
		iafstream& operator=(iafstream&&) noexcept = default;

		~iafstream() noexcept;

		// throws files::file_not_found and archive_error
		void open(const std::filesystem::path&);
		void close() noexcept;
		bool is_open() const noexcept;

		//throws archive_error and zip::file_not_found
		void open_file(const std::filesystem::path&);
		void close_file() noexcept;
		bool is_file_open() const noexcept;

		iafstream& read(char_t * buffer, std::size_t count);

		//sizes and positions are based on the uncompressed file
		//so usage of this class is the same as the equivalent ifstream on a normal file
		std::streamsize gcount() const noexcept;

		pos_type tellg();

		bool eof() const noexcept;

		void seekg(pos_type);
		void seekg(off_type, std::ios_base::seekdir);
	private:
		unarchive _archive = {};
		std::streamsize _gcount{};
		bool _file = false;
	};

	// out archive file stream
	class oafstream
	{
	public:
		using char_t = std::byte;
		using stream_t = std::ifstream;
		
		oafstream() noexcept = default;

		// throws archive_error
		explicit oafstream(const std::filesystem::path& p)
		{
			open(p);
			return;
		}

		oafstream(const oafstream&) = delete;
		oafstream& operator=(const oafstream&) = delete;

		oafstream(oafstream&&) noexcept = default;
		oafstream& operator=(oafstream&&) noexcept = default;

		~oafstream() noexcept
		{
			if (is_file_open())
				close_file();
			close();
			return;
		}

		// throws archive_error
		void open(const std::filesystem::path&);
		void close() noexcept;
		bool is_open() const noexcept;

		//throws archive_error and zip::file_not_found
		void open_file(const std::filesystem::path&);
		void close_file() noexcept;
		bool is_file_open() const noexcept
		{
			return !empty(_file);
		}

		oafstream& write(const char_t* buffer, std::streamsize count);

	private:
		toarchive _archive = {};
		string _file;
	};

	//in compressed file stream
	class izfstream
	{
	public:
		using char_t = std::byte;
		using stream_t = std::ifstream;
		using pos_type = stream_t::traits_type::pos_type;
		using off_type = stream_t::traits_type::off_type;

		izfstream() noexcept = default;
		explicit izfstream(const std::filesystem::path&);
		explicit izfstream(stream_t s); //stream will seek back to the begining

		//std::ifstream is not required to be noexcept movable
		izfstream(izfstream&&) noexcept(std::is_nothrow_move_constructible_v<std::ifstream>) = default;
		izfstream& operator=(izfstream&&) noexcept(std::is_nothrow_move_assignable_v<std::ifstream>) = default;

		~izfstream() noexcept;

		void open(const std::filesystem::path&);
		void close() noexcept;

		bool is_open() const noexcept
		{
			return _stream.is_open();
		}

		izfstream& read(char_t* buffer, std::size_t count);

		//sizes and positions are based on the uncompressed file
		//so usage of this class is the same as the equivalent ifstream on a normal file
		std::streamsize gcount() const
		{
			return _last_read;
		}

		pos_type tellg();

		bool eof() const
		{
			return _eof;
		}

		void seekg(pos_type);
		void seekg(off_type, std::ios_base::seekdir);

	private:
		buffer _buffer;
		bool _eof = false;
		std::size_t _buffer_pos = std::size_t{};
		std::size_t _buffer_end = std::size_t{};
		std::streamsize _last_read = std::streamsize{};
		std::unique_ptr<z_stream, void(*)(z_stream*)noexcept> _zip_stream;
		std::ifstream _stream;
	};

	static_assert(std::is_move_constructible_v<izfstream>);

	//ditermines if a file within an archive exists
	bool file_exists(const std::filesystem::path& archive, const std::filesystem::path& path);

	//returns all files in the archive within the dir_path directory, can continue recursively
	std::vector<types::string> list_files_in_archive(const std::filesystem::path& archive, const std::filesystem::path& dir, bool recursive = false);

	//compress_directory
	//path must be a directory
	//will create a zip in the parent directory with the same name
	void compress_directory(const std::filesystem::path& path);
	//uncompress archive
	void uncompress_archive(const std::filesystem::path& path);

	//test for zip compression header
	using zip_header = std::array<std::byte, 2u>;
	bool probably_compressed(zip_header) noexcept;
	bool probably_compressed(const buffer& data) noexcept;

	//both of these return archive exception on failure.
	buffer deflate(buffer uncompressed);
	buffer inflate(buffer compressed);
}

#endif // hades_data_hpp
