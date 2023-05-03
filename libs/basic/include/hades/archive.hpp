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

	std::string_view resource_archive_ext() noexcept;

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
		void open_archive(const std::filesystem::path&);
		void open(const std::filesystem::path&);
		void open_first();
		//returns true if another file was found
		bool open_next();
		void close();

		void close_archive() noexcept
		{
			_stream.close_archive();
			return;
		}

		bool is_open() const noexcept
		{
			return _stream.is_open();
		}

		bool is_archive_open() const noexcept
		{
			return _stream.is_archive_open();
		}

		const std::filesystem::path& archive_path() const noexcept
		{
			return _stream.archive_path();
		}

		const std::filesystem::path& file_path() const noexcept
		{
			return _stream.file_path();
		}

	private:
		in_archive_filebuf _stream;
	};

	// out archive file stream
	class oafstream : public std::ostream
	{
	public:
		oafstream() noexcept 
			: std::ostream{ &_stream }
		{}

		// throws archive_error
		explicit oafstream(const std::filesystem::path& p)
			: std::ostream{ &_stream }
		{
			open_archive(p);
			return;
		}

		oafstream(oafstream&& rhs) noexcept
			: std::ostream{ nullptr }
		{
			*this = std::move(rhs);
		}

		oafstream& operator=(oafstream&& rhs) noexcept
		{
			_stream = std::move(rhs._stream);
			rdbuf(&_stream);
			clear();
			return *this;
		}

		// throws archive_error
		void open_archive(const std::filesystem::path& p)
		{
			_stream.open_archive(p);
			clear();
		}

		void close_archive() noexcept
		{
			_stream.close_archive();
		}

		bool is_archive_open() const noexcept
		{
			return _stream.is_archive_open();
		}

		//throws archive_error and zip::archive_member_not_found
		void open(const std::filesystem::path& p)
		{
			_stream.open(p);
			clear();
			return;
		}

		void close() noexcept
		{
			_stream.close();
			return;
		}

		bool is_open() const noexcept
		{
			return _stream.is_open();
		}

	private:
		out_archive_filebuf _stream;
	};

	//in compressed file stream
	//reads possibly compressed files
	class izfstream : public std::istream
	{
	public:
		izfstream() noexcept;
		explicit izfstream(const std::filesystem::path&);
	
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
}

#endif // hades_data_hpp
