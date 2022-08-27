#ifndef HADES_ARCHIVE_STREAMS_HPP
#define HADES_ARCHIVE_STREAMS_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <streambuf>

#include "zlib.h"
#include "zip.h"
#include "unzip.h"
// please dont poison lower case names
#undef zlib_version

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

		basic_in_compressed_filebuf& operator=(basic_in_compressed_filebuf&& rhs) noexcept
		{
			std::swap(_get_area, rhs._get_area);
			std::swap(_device_buffer, rhs._device_buffer);
			std::swap(_file, rhs._file);
			std::swap(_buffer_pos, rhs._buffer_pos);
			std::swap(_buffer_end, rhs._buffer_end);
			std::swap(_zip_stream, rhs._zip_stream);

			// fix up the access ptrs
			auto ptr = rhs.gptr();
			auto ptr_offset = std::distance(rhs._get_area.data(), ptr);
			auto beg = _get_area.data();
			auto end = beg + size(_get_area);
			setg(beg, beg + ptr_offset, end);
			rhs.setg(nullptr, nullptr, nullptr);

			using z_byte = Bytef;
			using z_uint = uInt;

			// fixup the zip stream ptrs
			if (_zip_stream->next_in)
			{
				const auto in_distance = std::distance(rhs._get_area.data(), reinterpret_cast<char*>(_zip_stream->next_in));
				_zip_stream->next_in = reinterpret_cast<z_byte*>(_get_area.data() + in_distance);
			}

			if (_zip_stream->next_out)
			{
				const auto out_distance = std::distance(rhs._device_buffer.data(), reinterpret_cast<std::byte*>(_zip_stream->next_out));
				_zip_stream->next_out = reinterpret_cast<z_byte*>(_device_buffer.data() + out_distance);
			}
			return *this;
		}

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

		basic_in_compressed_filebuf* close() noexcept
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

		int_type underflow() final override
		{
			auto beg = eback(), ptr = gptr(), end = egptr();
			if (ptr != end)
				return *ptr;

			// this function depends on these types being used as generic byte ptr types
			static_assert(sizeof(char_type) == sizeof(Bytef));

			//uint as defined by zlib
			using z_uint = uInt;
			using z_byte = Bytef;
			auto eof = false;

			if (_zip_stream->avail_in < size(_device_buffer))
			{
				const auto device_begin = _device_buffer.data();
				const auto buffer_end = device_begin + size(_device_buffer);

				if (!_zip_stream->next_in)
				{
					_zip_stream->next_in = reinterpret_cast<z_byte*>(buffer_end);
				}

				const auto next_in = std::move(reinterpret_cast<std::byte*>(_zip_stream->next_in),
					buffer_end, device_begin);
				const auto dist = std::distance(next_in, buffer_end);
				const auto read_amount = std::fread(next_in, sizeof(std::byte), dist, _file);
				_buffer_pos = device_begin;
				_buffer_end = _buffer_pos + std::distance(_buffer_pos, next_in) + read_amount;

				_zip_stream->avail_in = integer_cast<z_uint>(std::distance(_buffer_pos, _buffer_end));
				_zip_stream->next_in = reinterpret_cast<z_byte*>(_buffer_pos);
			}

			// always has room from here
			_zip_stream->avail_out = integer_cast<z_uint>(size(_get_area));
			_zip_stream->next_out = reinterpret_cast<z_byte*>(beg);

			const auto ret = ::inflate(_zip_stream.get(), Z_NO_FLUSH);
			if (ret == Z_STREAM_END)
				eof = true;
			else if (ret != Z_OK)
			{
				auto msg = std::string{};
				if (_zip_stream->msg)
					msg = _zip_stream->msg;

				using namespace std::string_literals;

				switch (ret)
				{
				case Z_STREAM_ERROR:
					// generic error
					throw archive_error{ "error while reading compresed file; " + msg };
				case Z_DATA_ERROR:
					// corrupted data
					throw archive_error{ "compressed file was corrupt; " + msg };
				case Z_MEM_ERROR:
					// not enough memory
					throw archive_error{ "not enough memory to run decompression; " + msg };
				}
			}

			if (eof)
				end = reinterpret_cast<char_type*>(_zip_stream->next_out);

			setg(beg, beg, end);

			if (eof && beg == end)
				return traits_type::eof();

			return *beg;
		}

		int_type uflow() override
		{
			++_pos;
			return std::streambuf::uflow();
		}

	private:
		//seek back to the start of the file
		// requires reseting zlib and some of the other ptrs
		void _seek_beg() noexcept
		{
			if (_file)
			{
				auto beg = _get_area.data();
				auto ptr = beg + size(_get_area);
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

			return inflateInit(_zip_stream.get()) == Z_OK;
		}

		std::array<char_type, default_buffer_size> _get_area;
		std::array<std::byte, default_buffer_size> _device_buffer;
		std::unique_ptr<::z_stream> _zip_stream = std::make_unique<::z_stream>();
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

		basic_out_compressed_filebuf& operator=(basic_out_compressed_filebuf&& rhs) noexcept
		{
			std::swap(_put_area, rhs._put_area);
			std::swap(_device_buffer, rhs._device_buffer);
			std::swap(_file, rhs._file);
			std::swap(_zip_stream, rhs._zip_stream);

			// fix up the access ptrs
			auto ptr = rhs.pptr();
			auto ptr_offset = std::distance(rhs._put_area.data(), ptr);
			auto beg = _put_area.data();
			auto end = beg + size(_put_area);
			setp(beg, beg + ptr_offset, end);
			rhs.setp(nullptr, nullptr, nullptr);

			using z_byte = Bytef;
			using z_uint = uInt;

			// fixup the zip stream ptrs
			const auto in_distance = std::distance(rhs._put_area.data(), reinterpret_cast<char*>(_zip_stream->next_in));
			_zip_stream->next_in = reinterpret_cast<z_byte*>(_put_area.data() + in_distance);
			const auto out_distance = std::distance(rhs._device_buffer.data(), reinterpret_cast<std::byte*>(_zip_stream->next_out));
			_zip_stream->next_out = reinterpret_cast<z_byte*>(_device_buffer.data() + out_distance);

			return *this;
		}

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

		basic_out_compressed_filebuf* close()
		{
			if (_file)
			{
				deflate_some(Z_FINISH);
				sync();
				deflateEnd(_zip_stream.get());
				const auto ret = std::fclose(_file);
				_file = {};
				return ret == 0 ? this : nullptr;
			}
			return this;
		}

	// Writing funcs
	protected:
		int_type overflow(int_type = traits_type::eof())
		{
			const auto ptr = pptr(), end = epptr();
			if (ptr && ptr < end)
				return {};

			// this function depends on these types being used as generic byte ptr types
			static_assert(sizeof(char_type) == sizeof(Bytef));

			deflate_some();
			return {};
		}

		int_type sync()
		{
			const auto beg = pbase(), ptr = pptr();
			if (beg != ptr)
				deflate_some();

			auto ret = int_type{};

			if (_zip_stream->avail_out != size(_device_buffer))
			{
				using z_byte = Bytef;
				using z_uint = uInt;

				const auto device_end = reinterpret_cast<std::byte*>(_zip_stream->next_out);
				const auto write_amount = std::distance(_device_buffer.data(), device_end);
				const auto out = fwrite(_device_buffer.data(), sizeof(std::byte), write_amount, _file);
				if (out != integer_cast<std::size_t>(write_amount))
					throw files::file_error{ "write error" };
				const auto flush = fflush(_file);
				if (flush != int{})
					throw files::file_error{ "flush error" };

				_zip_stream->next_out = reinterpret_cast<z_byte*>(_device_buffer.data());
				_zip_stream->avail_out = integer_cast<z_uint>(size(_device_buffer));
			}

			return ret;
		}

	private:
		// deflates untill the put buffer is empty, only writes out to file as needed
		void deflate_some(int flush = Z_NO_FLUSH)
		{
			const auto beg = pbase(), ptr = pptr(), end = epptr();

			//uint as defined by zlib
			using z_uint = uInt;
			using z_byte = Bytef;
			_zip_stream->avail_in = integer_cast<z_uint>(std::distance(beg, ptr));
			_zip_stream->next_in = reinterpret_cast<z_byte*>(beg);

			auto cont = true;
			do
			{
				// check avail_out
				if (_zip_stream->avail_out == 0)
				{
					const auto write = fwrite(_device_buffer.data(), sizeof(std::byte), size(_device_buffer), _file);
					if (write < size(_device_buffer))
					{
						throw files::file_error{ "error while writing compressed data" };
					}

					_zip_stream->next_out = reinterpret_cast<z_byte*>(_device_buffer.data());
					_zip_stream->avail_out = integer_cast<z_uint>(size(_device_buffer));
				}

				const auto ret = deflate(_zip_stream.get(), flush);
				if (ret != Z_OK)
				{
					auto msg = std::string{};
					if (_zip_stream->msg)
						msg = _zip_stream->msg;

					switch (ret)
					{
					case Z_STREAM_ERROR:
						// generic error
						throw archive_error{ "error writing compressed file: Z_STREAM_ERROR; " + msg };
					case Z_DATA_ERROR:
						// corrupted data
						throw archive_error{ "error while compressing: Z_STREAM_ERROR; " + msg };
					case Z_MEM_ERROR:
						// not enough memory
						throw archive_error{ "not enough memory to run compression; " + msg };
					}
				}
				cont = (ret == Z_OK && _zip_stream->avail_out == 0);
			} while (cont);

			setp(beg, end);

			return;
		}

		bool _start_zlib() noexcept
		{
			using z_uint = uInt;
			using z_byte = Bytef;

			*_zip_stream = {};
			_zip_stream->zalloc = Z_NULL;
			_zip_stream->zfree = Z_NULL;
			_zip_stream->opaque = Z_NULL;
			_zip_stream->avail_in = 0;
			_zip_stream->next_in = reinterpret_cast<z_byte*>(_put_area.data());
			_zip_stream->avail_out = integer_cast<z_uint>(size(_device_buffer));
			_zip_stream->next_out = reinterpret_cast<z_byte*>(_device_buffer.data());

			return deflateInit(_zip_stream.get(), Z_BEST_COMPRESSION) == Z_OK;
		}

		std::array<char_type, default_buffer_size> _put_area;
		std::array<std::byte, default_buffer_size> _device_buffer;
		std::unique_ptr<::z_stream> _zip_stream = std::make_unique<::z_stream>();
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
		void _seek_beg()
		{
			assert(is_open());
			assert(is_file_open());
			unzCloseCurrentFile(_archive.handle);
			unzOpenCurrentFile(_archive.handle);
			_readsome();
			return;
		}

		// fill the input buffer
		int_type _readsome()
		{
			const auto ret = unzReadCurrentFile(_archive.handle, _get_area.data(), integer_cast<unsigned int>(size(_get_area)));

			using namespace std::string_literals;

			switch (ret)
			{
			case Z_ERRNO:
				[[fallthrough]];
			case Z_STREAM_ERROR:
				// generic error
				throw archive_error{ "error while reading compresed file"s };
			case Z_DATA_ERROR:
				// corrupted data
				throw archive_error{ "compressed file was corrupt"s };
			case Z_MEM_ERROR:
				// not enough memory
				throw archive_error{ "not enough memory to run decompression"s };
			}

			if (ret != 0)
			{
				const auto beg = _get_area.data();
				setg(beg, beg, beg + ret);
				return *beg;
			}
			else
				return traits_type::eof();
		}

		std::size_t _pos() const noexcept
		{
			const auto ptr = gptr(), end = egptr();
			return unztell64(_archive.handle) - std::distance(ptr, end);
		}

		std::array<char_type, default_buffer_size> _get_area;
		unarchive _archive;
	};


	using in_compressed_filebuf = basic_in_compressed_filebuf<char>;
	using out_compressed_filebuf = basic_out_compressed_filebuf<char>;

	using in_archive_filebuf = basic_in_archived_filebuf<char>;
}

#endif // !HADES_ARCHIVE_STREAMS_HPP
