#ifndef HADES_ARCHIVE_HPP
#define HADES_ARCHIVE_HPP

#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>

#include "hades/archive_streams.hpp"
#include "hades/exceptions.hpp"
#include "hades/string.hpp"

namespace hades::zip
{
	std::string_view zlib_version() noexcept;

	//open and close zip archives
	toarchive create_archive(const std::filesystem::path&);
	void close_archive(toarchive) noexcept;

	//in archive file stream
	class iafstream : public std::istream
	{
	public:
		iafstream() noexcept;
		explicit iafstream(const std::filesystem::path&);
		iafstream(const std::filesystem::path&, const std::filesystem::path&);

		iafstream(iafstream&&) noexcept;
		iafstream& operator=(iafstream&&) noexcept;

		// throws file_error
		void open(const std::filesystem::path&);
		void open_file(const std::filesystem::path&);
		void close_file();

		void close() noexcept
		{
			_stream.close();
			return;
		}

		bool is_open() const noexcept
		{
			return _stream.is_open();
		}

		bool is_file_open() const noexcept
		{
			return _stream.is_file_open();
		}

	private:
		in_archive_filebuf _stream;
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
	//reads possibly compressed files
	class izfstream : public std::istream
	{
	public:
		using char_t = char;
		using stream_t = std::ifstream;
		using pos_type = stream_t::traits_type::pos_type;
		using off_type = stream_t::traits_type::off_type;

		izfstream() noexcept;
		explicit izfstream(const std::filesystem::path&);
		//explicit izfstream(stream_t s); //stream will seek back to the begining

		izfstream(izfstream&&) noexcept;
		izfstream& operator=(izfstream&&) noexcept;

		void open(const std::filesystem::path&);
		void close() noexcept;

		bool is_open() const noexcept
		{
			return std::visit([](auto&& stream) {
				return stream.is_open();
				}, _stream);
		}

	private:
		std::variant<std::filebuf, in_compressed_filebuf> _stream;
	};
	
	static_assert(std::is_default_constructible_v<izfstream>);
	static_assert(std::is_move_constructible_v<izfstream>);
	static_assert(std::is_move_assignable_v<izfstream>);

	//out compressed file stream
	//writes compressed files
	class ozfstream : public std::ostream
	{
	public:
		ozfstream() noexcept;
		explicit ozfstream(const std::filesystem::path&);

		ozfstream(ozfstream&&) noexcept;
		ozfstream& operator=(ozfstream&&) noexcept;

		void open(const std::filesystem::path&);
		void close() noexcept;

		bool is_open() noexcept;

	private:
		out_compressed_filebuf _streambuf;
	};

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
	bool probably_compressed(const buffer& data) noexcept;

	//both of these return archive exception on failure.
	buffer deflate(buffer uncompressed);
	buffer inflate(buffer compressed);
}

#endif // hades_data_hpp
