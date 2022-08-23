#ifndef HADES_ARCHIVE_STREAMS_HPP
#define HADES_ARCHIVE_STREAMS_HPP

#include <array>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <streambuf>

#include "zlib.h"
// please dont poison lower case names
#undef zlib_version

#include "hades/exceptions.hpp"
#include "hades/utility.hpp"

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

	// stream buffer for reading or writing compressed files
	template<typename CharT, typename Traits = std::char_traits<CharT>>
	class basic_compressed_filebuf;
	
	// stream buffer for reading or writing compressed files
	// we only support basic char
	template<>
	class basic_compressed_filebuf<char, std::char_traits<char>> : public std::basic_streambuf<char, std::char_traits<char>>
	{
	public:
		enum class open_mode {
			read,
			write
		};

		basic_compressed_filebuf(const std::filesystem::path& p, open_mode o)
		{
			open(p, o);
			return;
		}

		basic_compressed_filebuf(basic_compressed_filebuf&& rhs) noexcept
		{
			*this = std::move(rhs);
			return;
		}

		basic_compressed_filebuf& operator=(basic_compressed_filebuf&& rhs) noexcept
		{
			std::swap(_get_put_area, rhs._get_put_area);
			std::swap(_device_buffer, rhs._device_buffer);
			std::swap(_mode, rhs._mode);
			std::swap(_file, rhs._file);
			std::swap(_buffer_pos, rhs._buffer_pos);
			std::swap(_buffer_end, rhs._buffer_end);
			std::swap(_zip_stream, rhs._zip_stream);

			// fix up the access ptrs
			auto ptr = rhs.gptr();
			auto ptr_offset = std::distance(rhs._get_put_area.data(), ptr);
			auto beg = _get_put_area.data();
			auto end = beg + size(_get_put_area);
			setg(beg, beg + ptr_offset, end);
			rhs.setg(nullptr, nullptr, nullptr);

			//TODO: recheck this after adding write support
			return *this;
		}

		~basic_compressed_filebuf() noexcept
		{
			close();
			return;
		}

		bool is_open() const noexcept
		{
			return _file;
		}

		basic_compressed_filebuf* open(const std::filesystem::path& p, open_mode o) noexcept
		{
			if (_file)
				return nullptr;

			const auto p_str = p.string();
			_file = std::fopen(p_str.c_str(), o == open_mode::read ? "rb" : "wb");
			if (!_file)
				return nullptr;
			if (o == open_mode::read)
			{
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
			}
			else if (o == open_mode::write)
			{
				assert(false);
			}

			auto beg = _get_put_area.data();
			auto ptr = beg + size(_get_put_area);
			setg(beg, ptr, ptr);

			return this;
		}

		basic_compressed_filebuf* close() noexcept
		{
			if (_file)
			{
				inflateEnd(_zip_stream.get());
				const auto ret = std::fclose(_file);
				_file = {};
				return ret == 0 ? this : nullptr;
			}
			return this;
		}

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
					seekoff(_pos + off, std::ios_base::beg);
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
					while (_pos < off)
						uflow();
					break;
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

		int_type underflow() final override
		{
			assert(_mode == open_mode::read);
			auto beg = eback(), ptr = gptr(), end = egptr();
			if (ptr < end)
				return *ptr;

			// this function depends on these types being used as generic byte ptr types
			static_assert(sizeof(char_type) == sizeof(Bytef));

			//uint as defined by zlib
			using z_uint = uInt;

			if (_zip_stream->avail_in < size(_device_buffer))
			{
				const auto device_begin = _device_buffer.data();
				const auto buffer_end = device_begin + size(_device_buffer);

				if (!_zip_stream->next_in)
				{
					_zip_stream->next_in = reinterpret_cast<Bytef*>(buffer_end);
				}

				const auto next_in = std::move(reinterpret_cast<std::byte*>(_zip_stream->next_in),
					buffer_end, device_begin);
				const auto dist = std::distance(next_in, buffer_end);
				const auto read_amount = std::fread(next_in, sizeof(std::byte), dist, _file);
				_buffer_pos = device_begin;
				_buffer_end = _buffer_pos + std::distance(_buffer_pos, next_in) + read_amount;

				_zip_stream->avail_in = integer_cast<z_uint>(std::distance(_buffer_pos, _buffer_end));
				_zip_stream->next_in = reinterpret_cast<Bytef*>(_buffer_pos);
			}

			// always has room from here
			_zip_stream->avail_out = integer_cast<z_uint>(std::distance(beg, end));
			_zip_stream->next_out = reinterpret_cast<Bytef*>(beg);
			ptr = beg;

			const auto ret = ::inflate(_zip_stream.get(), Z_NO_FLUSH);
			if (ret == Z_STREAM_END)
				_eof = true;
			else if (ret != Z_OK)
				throw archive_error{ "failed to inflate during read" };

			/*_last_read = integer_cast<std::streamsize>(count) -
				integer_cast<std::streamsize>(_zip_stream->stream.avail_out);*/

			if (_eof)
				end = reinterpret_cast<char_type*>(_zip_stream->next_out);

			setg(beg, ptr, end);

			if (_eof && ptr == end)
				return traits_type::eof();

			return *beg;
		}

		int_type uflow() override
		{
			++_pos;
			return std::streambuf::uflow();
		}

	// putback
	protected:
		/*int_type pbackfail(int_type c = traits_type::eof()) override
		{

		}*/

	private:
		//seek back to the start of the file
		// requires reseting zlib and some of the other ptrs
		void _seek_beg() noexcept
		{
			if (_file)
			{
				auto beg = _get_put_area.data();
				auto ptr = beg + size(_get_put_area);
				setg(beg, ptr, ptr);
				rewind(_file);
				_pos = {};
				inflateEnd(_zip_stream.get());
				_start_zlib();
			}
			return;
		}

		bool _start_zlib() noexcept
		{
			*_zip_stream = {};
			_zip_stream->zalloc = Z_NULL;
			_zip_stream->zfree = Z_NULL;
			_zip_stream->opaque = Z_NULL;
			_zip_stream->avail_in = 0;
			_zip_stream->next_in = Z_NULL;
			const auto ret = inflateInit(_zip_stream.get()) == Z_OK;

			return ret;
		}

		std::array<char_type, default_buffer_size> _get_put_area;
		std::array<std::byte, default_buffer_size> _device_buffer;
		open_mode _mode = {};
		FILE* _file = {};
		std::streamsize _pos = {};

		bool _eof = false;
		std::byte* _buffer_pos = {};
		std::byte* _buffer_end = {};
		std::unique_ptr<::z_stream> _zip_stream = std::make_unique<::z_stream>();
	};

	using compressed_filebuf = basic_compressed_filebuf<char>;
	static_assert(std::is_move_constructible_v<compressed_filebuf>);
}

#endif // !HADES_ARCHIVE_STREAMS_HPP
