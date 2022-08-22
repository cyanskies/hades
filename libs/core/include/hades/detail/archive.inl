#include "hades/archive.hpp"

#include <cassert>

#include "zlib.h"

namespace hades::zip
{
	// stream buffer for reading or writing compressed files
	template<typename CharT, typename Traits>
	class basic_compressed_filebuf : public std::basic_streambuf<CharT, Traits>
	{
	public:
		enum class open_mode {
			read,
			write
		};

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
			_file = std::fopen(p.c_str(), o == open_mode::read ? "rb" : "wb");
			if (!_file)
				return nullptr;
			if (o == open_mode::read)
			{
				auto header = zip_header{};
				const auto read_count = std::fread(header.data(), sizeof(zip_header::value_type), size(header), _file);
				std::rewind(_file);
				if (read_count != size(header) || !probably_compressed(zip_header))
					return nullptr;

				_zip_stream.zalloc = Z_NULL;
				_zip_stream.zfree = Z_NULL;
				_zip_stream.opaque = Z_NULL;
				_zip_stream.avail_in = 0;
				_zip_stream.next_in = Z_NULL;
				if (inflateInit(&stream) != Z_OK)
					return nullptr;
			}
			else if (o == open_mode::write)
			{
				assert(false);
			}

			return this;
		}

		basic_compressed_filebuf* close() noexcept
		{
			inflateEnd(&_zip_stream);
			_zip_stream = {};
			const auto ret = std::fclose(_file);
			_file = {};
			return ret == 0 ? this : nullptr;
		}

	protected:
		std::streamsize showmanyc() final noexcept override
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
			
			//uint as defined by zlib
			using z_uint = uInt;

			if (_zip_stream->stream.avail_in < size(_compression_buffer))
			{
				const auto buffer_end = _compression_buffer.data() + size(_compression_buffer);
				const auto next_in = std::move(_zip_stream->stream.next_in,
					buffer_end, _compression_buffer.data());
				const auto read_amount = std::fread(next_in, sizeof(std::byte), std::distance(next_in, buffer_end));
				_buffer_pos = _compression_buffer.data();
				_buffer_end = _buffer_pos + read_amount;

				_zip_stream->stream.avail_in = integer_cast<z_uint>(std::distance(_buffer_pos, _buffer_end));
				_zip_stream->stream.next_in = reinterpret_cast<Bytef*>(_buffer_pos);
			}

			// always has room from here
			_zip_stream->stream.avail_out = integer_cast<z_uint>(std::distance(beg, end));
			_zip_stream->stream.next_out = reinterpret_cast<Bytef*>(beg);
			ptr = beg;

			const auto ret = ::inflate(&_zip_stream->stream, Z_NO_FLUSH);
			if (ret == Z_STREAM_END)
				_eof = true;
			else if (ret != Z_OK)
				throw archive_error{ "failed to inflate during read"s };

			_last_read = integer_cast<std::streamsize>(count) -
				integer_cast<std::streamsize>(_zip_stream->stream.avail_out);

			if (_eof)
				end = _zip_stream->stream.next_out;

			setg(beg, ptr, end);

			return *beg;
		}

	private:
		std::array<CharT, default_buffer_size> _get_put_area;
		std::array<std::byte, default_buffer_size> _compression_buffer;
		open_mode _mode = {};

		FILE* _file = {};

		bool _eof = false;
		std::byte* _buffer_pos = {};
		std::byte* _buffer_end = {};
		std::streamsize _last_read = std::streamsize{};
		z_stream _zip_stream = {};
	};
}