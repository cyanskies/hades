#ifndef HADES_ARCHIVE_STREAMS_HPP
#define HADES_ARCHIVE_STREAMS_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <streambuf>

#include "hades/exceptions.hpp"
#include "hades/logging.hpp"
#include "hades/utility.hpp"

/// <summary>
/// Low level stream utilities for compressed files and zip archives
/// </summary>

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

	namespace detail
	{
		struct z_stream;
		using z_stream_p = std::unique_ptr<z_stream, void(*)(z_stream*)noexcept>;
		z_stream_p make_z_stream();
	}

	//test for zip compression header
	using zip_header = std::array<std::byte, 2u>;
	namespace header
	{
		constexpr std::byte first{ 0x78 };
		constexpr std::array<std::byte, 3> others{
			static_cast<std::byte>(0x01),
			static_cast<std::byte>(0x9C),
			static_cast<std::byte>(0xDA)
		};
	}

	constexpr bool probably_compressed(zip_header header) noexcept
	{
		return header[0] == header::first &&
			std::any_of(std::begin(header::others), std::end(header::others), [second = header[1]](auto&& other)
				{
					return second == other;
				});
	}

	struct unarchive
	{
		std::filesystem::path path;
		std::filesystem::path file;
		void* handle = {};
	};

	//manipulate unzip archive streams
	// throws: file_error, file_not_found, archive_error
	unarchive open_archive(const std::filesystem::path&);

	// throws file_not_found if file_exists would return false for this path
	void open_file(unarchive&, const std::filesystem::path&);
	void open_first_file(unarchive&);
	// returns true if successful
	// returns false if reaching the end
	bool open_next_file(unarchive&);
	void close_file(unarchive&);
	void close_archive(unarchive&) noexcept;
	bool file_exists(const unarchive&, const std::filesystem::path&) noexcept;

	struct toarchive
	{
		string path;
		void* handle = {};
	};

	// stream buffer for reading compressed files
	template<typename CharT, typename Traits = std::char_traits<CharT>>
	class basic_in_compressed_filebuf;
	
	// stream buffer for writing compressed files
	template<typename CharT, typename Traits = std::char_traits<CharT>>
	class basic_out_compressed_filebuf;

	// stream buffer for reading from archives
	template<typename CharT, typename Traits = std::char_traits<CharT>>
	class basic_in_archived_filebuf;

	// stream buffer for writing archives
	template<typename CharT, typename Traits = std::char_traits<CharT>>
	class basic_out_archived_filebuf;

	// stream buffer for reading or writing compressed files
	// we only support basic char
	template<>
	class basic_in_compressed_filebuf<char> : public std::basic_streambuf<char>
	{
	public:
		basic_in_compressed_filebuf() noexcept = default;

		basic_in_compressed_filebuf(const std::filesystem::path& p)
		{
			open(p);
			return;
		}

		basic_in_compressed_filebuf(basic_in_compressed_filebuf&& rhs) noexcept
		{
			*this = std::move(rhs);
			return;
		}

		basic_in_compressed_filebuf& operator=(basic_in_compressed_filebuf&& rhs) noexcept;

		~basic_in_compressed_filebuf() noexcept
		{
			close();
			return;
		}

		bool is_open() const noexcept
		{
			return _file;
		}

		basic_in_compressed_filebuf* open(const std::filesystem::path& p)
		{
			if (_file)
				return nullptr;

			const auto p_str = p.generic_string();
			_file = std::fopen(p_str.c_str(), "rb");
			if (!_file)
				return nullptr;
			
			auto header = zip_header{};
			const auto read_count = std::fread(header.data(), sizeof(zip_header::value_type), size(header), _file);
			std::rewind(_file);
			if (read_count != size(header) || !probably_compressed(header))
			{
				std::fclose(_file);
				_file = {};
				return nullptr;
			}

			if (!_start_zlib())
			{
				std::fclose(_file);
				_file = {};
				return nullptr;
			}
			
			auto beg = _get_area.data();
			auto ptr = beg + size(_get_area);
			setg(beg, ptr, ptr);

			return this;
		}

		basic_in_compressed_filebuf* close() noexcept;

	// Seeking functions
	protected:
		pos_type seekoff(off_type off, std::ios_base::seekdir dir,
			std::ios_base::openmode = std::ios_base::in | std::ios_base::out) noexcept final override
		{
			if (off < 0)
			{
				switch (dir)
				{
				case std::ios_base::beg:
					_seek_beg();
					break;
				case std::ios_base::end:
					while (uflow() != traits_type::eof()); 
					[[fallthrough]];
				case std::ios_base::cur:
					auto final = _pos + off;
					if (final < 0)
						seekoff(0, std::ios_base::beg);
					else
						seekoff(false, std::ios_base::beg);
				}
			}
			else
			{
				switch (dir)
				{
				case std::ios_base::beg:
					_seek_beg();
					[[fallthrough]];
				case std::ios_base::cur:
				{
					const auto end = _pos + off;
					while (_pos < end && uflow() != traits_type::eof());
				} break;
				case std::ios_base::end:
					while (uflow() != traits_type::eof());
				}
			}

			return _pos;
		}

		pos_type seekpos(pos_type pos,
			std::ios_base::openmode = std::ios_base::in | std::ios_base::out) noexcept final override
		{
			return seekoff(std::streamoff{ pos }, std::ios_base::beg);
		}

	// Reading functions
	protected:
		std::streamsize showmanyc() noexcept final override
		{
			const auto ptr = gptr(), end = egptr();
			return std::distance(ptr, end);
		}

		int_type underflow() final override;

		int_type uflow() override
		{
			++_pos;
			return std::streambuf::uflow();
		}

	private:
		//seek back to the start of the file
		// requires reseting zlib and some of the other ptrs
		void _seek_beg() noexcept;
		bool _start_zlib() noexcept;

		std::array<char_type, default_buffer_size> _get_area;
		std::array<std::byte, default_buffer_size> _device_buffer;
		detail::z_stream_p _zip_stream = detail::make_z_stream();
		FILE* _file = {};
		std::byte* _buffer_pos = {};
		std::byte* _buffer_end = {};
		std::streamsize _pos = {};
	};

	// stream buffer for writing compressed files
	// we only support basic char
	template<>
	class basic_out_compressed_filebuf<char> : public std::basic_streambuf<char>
	{
	public:
		basic_out_compressed_filebuf() noexcept = default;

		basic_out_compressed_filebuf(const std::filesystem::path& p)
		{
			open(p);
			return;
		}

		basic_out_compressed_filebuf(basic_out_compressed_filebuf&& rhs) noexcept
		{
			*this = std::move(rhs);
			return;
		}

		basic_out_compressed_filebuf& operator=(basic_out_compressed_filebuf&& rhs) noexcept;

		~basic_out_compressed_filebuf() noexcept
		{
			close();
			return;
		}

		bool is_open() const noexcept
		{
			return _file;
		}

		basic_out_compressed_filebuf* open(const std::filesystem::path& p)
		{
			using namespace std::string_literals;

			if (_file)
				throw files::file_error{ "Tried to open a file for compressed output with an already opened stream"s };

			const auto p_str = p.generic_string();
			_file = std::fopen(p_str.c_str(), "wb");
			if (!_file)
				return nullptr;

			if (!_start_zlib())
			{
				std::fclose(_file);
				_file = {};
				return nullptr;
			}

			const auto beg = _put_area.data();
			const auto end = beg + size(_put_area);
			setp(beg, end);

			return this;
		}

		basic_out_compressed_filebuf* close();

	// Writing funcs
	protected:
		int_type overflow(int_type = traits_type::eof());
		int_type sync();

	private:
		// deflates until the put buffer is empty, only writes out to file as needed
		void deflate_some(int flush);
		bool _start_zlib() noexcept;

		std::array<char_type, default_buffer_size> _put_area;
		std::array<std::byte, default_buffer_size> _device_buffer;
		detail::z_stream_p _zip_stream = detail::make_z_stream();
		FILE* _file = {};
	};

	// stream buffer for reading from archives
	template<>
	class basic_in_archived_filebuf<char> : public std::basic_streambuf<char>
	{
	public:
		basic_in_archived_filebuf() noexcept = default;

		basic_in_archived_filebuf(basic_in_archived_filebuf&& rhs) noexcept
		{
			*this = std::move(rhs);
			return;
		}

		basic_in_archived_filebuf& operator=(basic_in_archived_filebuf&& rhs) noexcept
		{
			std::swap(_archive, rhs._archive);
			std::swap(_get_area, rhs._get_area);

			// fix up the access ptrs
			auto ptr = rhs.pptr();
			auto ptr_offset = std::distance(rhs._get_area.data(), ptr);
			auto beg = _get_area.data();
			auto end = beg + size(_get_area);
			setp(beg, beg + ptr_offset, end);
			rhs.setp(nullptr, nullptr, nullptr);

			return *this;
		}

		~basic_in_archived_filebuf() noexcept
		{
			if (is_file_open())
			{
				try
				{
					close_file();
				}
				catch (const archive_error& e)
				{
					log_warning(e.what());
				}
				close();
			}
			else if(is_open())
				close();
			return;
		}

		bool is_open() const noexcept
		{
			return _archive.handle;
		}

		bool is_file_open() const noexcept
		{
			return !_archive.file.empty();
		}

		basic_in_archived_filebuf* open(const std::filesystem::path& p)
		{
			assert(!is_open());
			_archive = open_archive(p);
			if(_archive.handle)
				return this;
			return nullptr;
		}

		basic_in_archived_filebuf* open_file(const std::filesystem::path& p)
		{
			assert(is_open());
			assert(!is_file_open());
			zip::open_file(_archive, p);
			_readsome();
			return this;
		}

		basic_in_archived_filebuf* close() noexcept
		{
			close_archive(_archive); 
			return this;
		}

		// throws archive_error
		basic_in_archived_filebuf* close_file()
		{
			assert(is_file_open());
			zip::close_file(_archive);
			return this;
		}

		// Seeking functions
	protected:
		pos_type seekoff(off_type off, std::ios_base::seekdir dir,
			std::ios_base::openmode = std::ios_base::in | std::ios_base::out) noexcept final override
		{
			assert(is_file_open());

			if (off < 0)
			{
				switch (dir)
				{
				case std::ios_base::beg:
					_seek_beg();
					break;
				case std::ios_base::end:
					while (uflow() != traits_type::eof());
					[[fallthrough]];
				case std::ios_base::cur:
					const auto final = _pos() + off;
					if (final < 0)
						seekoff(0, std::ios_base::beg);
					else
						seekoff(final, std::ios_base::beg);
				}
			}
			else
			{
				switch (dir)
				{
				case std::ios_base::beg:
					_seek_beg();
					[[fallthrough]];
				case std::ios_base::cur:
				{
					auto pos = integer_cast<std::streamoff>(_pos());
					const auto end = pos + off;
					while (++pos < end && uflow() != traits_type::eof());
				} break;
				case std::ios_base::end:
					while (uflow() != traits_type::eof());
				}
			}

			return _pos();
		}

		pos_type seekpos(pos_type pos,
			std::ios_base::openmode = std::ios_base::in | std::ios_base::out) noexcept final override
		{
			assert(is_file_open());
			return seekoff(std::streamoff{ pos }, std::ios_base::beg);
		}

		// Reading functions
	protected:
		std::streamsize showmanyc() noexcept final override
		{
			const auto ptr = gptr(), end = egptr();
			return std::distance(ptr, end);
		}

		int_type underflow() final override
		{
			const auto ptr = gptr(), end = egptr();
			if (ptr != end)
				return *ptr;

			return _readsome();
		}

	private:
		void _seek_beg();
		// fill the input buffer
		int_type _readsome();
		std::size_t _pos() const noexcept;

		std::array<char_type, default_buffer_size> _get_area;
		unarchive _archive;
	};


	using in_compressed_filebuf = basic_in_compressed_filebuf<char>;
	using out_compressed_filebuf = basic_out_compressed_filebuf<char>;

	using in_archive_filebuf = basic_in_archived_filebuf<char>;
}

#endif // !HADES_ARCHIVE_STREAMS_HPP
