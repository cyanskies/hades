#ifndef HADES_ARCHIVE_STREAMS_HPP
#define HADES_ARCHIVE_STREAMS_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <span>
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
	// TODO: deprecate
	using buffer = std::vector<std::byte>;
	//NOTE: buffer sizes:
	//					16kb(16384)
	//					8kb(8192)
	//					4kb(4096)
	//					2kb(2048)
	constexpr auto default_buffer_size = std::size_t{ 8192 };

	namespace detail
	{
		template<typename CharT>
		constexpr std::vector<CharT> make_default_buffer()
		{
			return std::vector<CharT>(default_buffer_size, CharT{});
		}
	}
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
}

namespace hades::zip
{
	class archive_error : public files::file_error
	{
	public:
		using file_error::file_error;
	};

	class archive_member_not_found : public archive_error
	{
	public:
		using archive_error::archive_error;
	};

	namespace detail
	{
		struct z_stream;
		struct z_stream_deleter
		{
			void operator()(z_stream*) const noexcept;
		};

		using z_stream_p = std::unique_ptr<z_stream, z_stream_deleter>;
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

	constexpr bool probably_compressed(std::span<const std::byte, 2> header) noexcept
	{
		switch (static_cast<char>(header[1]))
		{
		case static_cast<char>(header::others[0]):
			[[fallthrough]];
		case static_cast<char>(header::others[1]):
			[[fallthrough]];
		case static_cast<char>(header::others[2]):
			return header[0] == header::first;
		default:
			return false;
		}
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
		std::filesystem::path path;
		std::filesystem::path file;
		void* handle = {};
	};

	//open and close zip archives
	toarchive create_archive(const std::filesystem::path&);
	void open_file(toarchive&, const std::filesystem::path&);
	void close_file(toarchive&) noexcept;
	void close_archive(toarchive&) noexcept;

	namespace detail
	{
		struct file_closer
		{
			void operator()(FILE*) const noexcept;
		};
		using file_ptr = std::unique_ptr<FILE, file_closer>;
		file_ptr open_file(const char*, const char*) noexcept;
	}

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
			return _file.get();
		}

		basic_in_compressed_filebuf* open(const std::filesystem::path& p)
		{
			if (_file)
				return nullptr;

			const auto p_str = p.generic_string();
			_file = detail::open_file(p_str.c_str(), "rb");
			if (!_file)
				return nullptr;
			
			auto header = zip_header{};
			const auto read_count = std::fread(header.data(), sizeof(zip_header::value_type), size(header), _file.get());
			std::rewind(_file.get());
			if (read_count != size(header) || !probably_compressed(header))
			{
				_file = {};
				return nullptr;
			}

			if (!_start_zlib())
			{
				_file = {};
				return nullptr;
			}
			
			_fill_buffer();
			return this;
		}

		basic_in_compressed_filebuf* close() noexcept;

	// Seeking functions
	protected:
		pos_type seekoff(const off_type off, const std::ios_base::seekdir dir,
			const std::ios_base::openmode = std::ios_base::in | std::ios_base::out) noexcept final override
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
					const auto final = integer_cast<off_type>(_pos) + off;
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

		int_type pbackfail(int_type c = traits_type::eof()) final override
		{
			auto old_pos = _pos;
			_seek_beg();
			seekpos(old_pos - 1);
			if (c != traits_type::eof())
				_get_area[0] = traits_type::to_char_type(c);
			return !traits_type::eof();
		}

	private:
		int_type _fill_buffer();
		//seek back to the start of the file
		// requires reseting zlib and some of the other ptrs
		void _seek_beg() noexcept;
		bool _start_zlib() noexcept;

		std::vector<char_type> _get_area = hades::detail::make_default_buffer<char_type>();
		std::vector<std::byte> _device_buffer = hades::detail::make_default_buffer<std::byte>();
		detail::z_stream_p _zip_stream = detail::make_z_stream();
		detail::file_ptr _file;
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

		~basic_out_compressed_filebuf() noexcept final override
		{
			close();
			return;
		}

		bool is_open() const noexcept
		{
			return _file.get();
		}

		basic_out_compressed_filebuf* open(const std::filesystem::path& p)
		{
			using namespace std::string_literals;

			if (_file)
				throw files::file_error{ "Tried to open a file for compressed output with an already opened stream"s };

			const auto p_str = p.generic_string();
			_file = detail::open_file(p_str.c_str(), "wb");
			if (!_file)
				return nullptr;

			if (!_start_zlib())
			{
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
		int_type overflow(int_type = traits_type::eof()) final override;
		int_type sync() final override;

	private:
		// deflates until the put buffer is empty, only writes out to file as needed
		void _deflate_some(int flush);
		void _empty_device_buffer(); 
		bool _start_zlib() noexcept;

		std::vector<char_type> _put_area = hades::detail::make_default_buffer<char_type>();
		std::vector<std::byte> _device_buffer = hades::detail::make_default_buffer<std::byte>();
		detail::z_stream_p _zip_stream = detail::make_z_stream();
		detail::file_ptr _file;
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
			auto ptr = rhs.gptr();
			auto end = rhs.egptr();
			auto ptr_offset = std::distance(rhs._get_area.data(), ptr);
			auto end_offset = std::distance(rhs._get_area.data(), end);
			auto beg = _get_area.data();
			setg(beg, beg + ptr_offset, beg + end_offset);
			rhs.setg(nullptr, nullptr, nullptr);

			return *this;
		}

		~basic_in_archived_filebuf() noexcept
		{
			if (is_open())
			{
				try
				{
					close();
				}
				catch (const archive_error& e)
				{
					log_warning(e.what());
				}
				close_archive();
			}
			else if(is_archive_open())
				close_archive();
			return;
		}

		bool is_archive_open() const noexcept
		{
			return _archive.handle;
		}

		bool is_open() const noexcept
		{
			return !_archive.file.empty();
		}

		basic_in_archived_filebuf* open_archive(const std::filesystem::path& p)
		{
			assert(!is_open());
			_archive = zip::open_archive(p);
			if(_archive.handle)
				return this;
			return nullptr;
		}

		basic_in_archived_filebuf* open(const std::filesystem::path& p)
		{
			assert(is_archive_open());
			assert(!is_open());
			zip::open_file(_archive, p);
			_readsome();
			return this;
		}

		basic_in_archived_filebuf* open_first()
		{
			assert(is_archive_open());
			assert(!is_open());
			zip::open_first_file(_archive);
			_readsome();
			return this;
		}

		basic_in_archived_filebuf* open_next()
		{
			assert(is_archive_open());
			assert(!is_open());
			if (zip::open_next_file(_archive))
			{
				_readsome();
				return this;
			}

			return nullptr;
		}

		basic_in_archived_filebuf* close_archive() noexcept
		{
			zip::close_archive(_archive); 
			return this;
		}

		// throws archive_error
		basic_in_archived_filebuf* close()
		{
			assert(is_archive_open());
			zip::close_file(_archive);
			return this;
		}

		const std::filesystem::path& archive_path() const noexcept
		{
			return _archive.path;
		}

		const std::filesystem::path& file_path() const noexcept
		{
			return _archive.file;
		}

		// Seeking functions
	protected:
		pos_type seekoff(const off_type off, const std::ios_base::seekdir dir,
			const std::ios_base::openmode = std::ios_base::in | std::ios_base::out) noexcept final override
		{
			assert(is_open());

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
                    const auto final = integer_cast<off_type>(_pos()) + off;
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

            return integer_cast<std::streamoff>(_pos());
		}

		pos_type seekpos(pos_type pos,
			std::ios_base::openmode = std::ios_base::in | std::ios_base::out) noexcept final override
		{
			assert(is_open());
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
				return traits_type::to_int_type(*ptr);

			return _readsome();
		}

	private:
		void _seek_beg();
		// fill the input buffer
		int_type _readsome();
		std::size_t _pos() const noexcept;

		std::vector<char_type> _get_area = hades::detail::make_default_buffer<char_type>();
		unarchive _archive;
	};

	// stream buffer for writing compressed files
	// we only support basic char
	template<>
	class basic_out_archived_filebuf<char> : public std::basic_streambuf<char>
	{
	public:
		basic_out_archived_filebuf() noexcept = default;

		basic_out_archived_filebuf(basic_out_archived_filebuf&& rhs) noexcept
		{
			*this = std::move(rhs);
			return;
		}

		basic_out_archived_filebuf& operator=(basic_out_archived_filebuf&& rhs) noexcept
		{

            std::swap(_archive, rhs._archive);

            // measure the rhs put area usage
            const auto ptr = rhs.pptr();
            const auto ptr_offset = integer_cast<int>(unsigned_cast(std::distance(rhs._put_area.data(), ptr)) / sizeof(char_type));

            // clear the rhs area
            rhs.setp(nullptr, nullptr);
            // swap the current put data
            std::swap(_put_area, rhs._put_area);

            // calculate the new ptrs for our put area
            auto beg = _put_area.data();
			auto end = beg + size(_put_area);
            setp(beg, end);
            // advance the put pointer to match rhs
            pbump(ptr_offset);

            return *this;
		}

		~basic_out_archived_filebuf() noexcept final override
		{
			close_archive();
			return;
		}

		bool is_open() const noexcept
		{
			return !_archive.file.empty();
		}

		bool is_archive_open() const noexcept
		{
			return _archive.handle;
		}

		basic_out_archived_filebuf* open_archive(const std::filesystem::path& p)
		{
			assert(!is_archive_open());
			_archive = zip::create_archive(p);
			return this;
		}

		void close_archive() noexcept
		{
			if(is_open())
				close();
			zip::close_archive(_archive);
			return;
		}

		basic_out_archived_filebuf* open(const std::filesystem::path& p)
		{
			assert(is_archive_open());
			assert(!is_open());
			zip::open_file(_archive, p);
			auto beg = _put_area.data();
			setp(beg, beg + size(_put_area));
			return this;
		}

		void close()
		{
			assert(is_archive_open());
			assert(is_open());
			sync();
			zip::close_file(_archive);
			return;
		}

		// Writing funcs
	protected:
		int_type overflow(int_type i = traits_type::eof()) final override
		{
			const auto ptr = pptr(), end = epptr();
			if (ptr == end)
				_consume_buffer();

			if (i != traits_type::eof())
			{
				const auto beg = pbase();
				*beg = traits_type::to_char_type(i);
                setp(beg, end);
                pbump(1);
			}

			return !traits_type::eof();
		}

		int_type sync() final override
		{
			_consume_buffer();
			return {};
		}

	private:
		void _consume_buffer();

		std::vector<char_type> _put_area = hades::detail::make_default_buffer<char_type>();
		toarchive _archive;
	};

	using in_compressed_filebuf = basic_in_compressed_filebuf<char>;
	using out_compressed_filebuf = basic_out_compressed_filebuf<char>;

	using in_archive_filebuf = basic_in_archived_filebuf<char>;
	using out_archive_filebuf = basic_out_archived_filebuf<char>;
}

#endif // !HADES_ARCHIVE_STREAMS_HPP
