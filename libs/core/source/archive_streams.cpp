#include "hades/archive_streams.hpp"

#include <fstream>
#include <set>

#include "zlib.h"
#include "zip.h"
#include "unzip.h"
// please dont poison lower case names
#undef zlib_version

#include "hades/string.hpp"

namespace fs = std::filesystem;

using namespace std::string_literals;
using namespace std::string_view_literals;

//this is the default archive extension
//used when created archives, but not when searching for them
constexpr auto archive_ext = ".zip"sv;

namespace hades::zip::detail
{
	struct z_stream
	{
		::z_stream z = {};
	};

	static void delete_z_stream(z_stream* z) noexcept
	{
		delete z;
		return;
	}

	z_stream_p make_z_stream()
	{
		return z_stream_p{ new z_stream{}, delete_z_stream };
	}
}

namespace hades::zip
{
	// list of open archive handles
	static std::multiset<string> open_for_read;
	static std::set<string> open_for_write;

	unarchive open_archive(const fs::path& path)
	{
		auto path_str = path.generic_string();
		if (open_for_write.contains(path_str))
			throw files::file_error{ "Cannot open archive: " + path_str + ", it is already open for writing"s };

		if (!fs::exists(path))
			throw files::file_not_found{ "archive not found: "s + path.generic_string() };
		//open archive
		auto zip = unzOpen64(path_str.c_str());

		if (!zip)
			return {};

		open_for_read.emplace(path_str);

		return { path, {}, zip };
	}

	void open_file(unarchive& a, const std::filesystem::path& f)
	{
		assert(a.handle);

		//sets current file to the target if it returns true
		if (!file_exists(a, f))
			throw file_not_found{ "file not found in archive: "s + f.generic_string() };

		if (unzOpenCurrentFile(a.handle) != UNZ_OK)
			throw archive_error{ "error opening file in archive"s };

		a.file = f;
		return;
	}

	void open_first_file(unarchive& a)
	{
		assert(a.file.empty());
		if (UNZ_OK != unzGoToFirstFile(a.handle))
			throw archive_error{ "error opening file in archive"s };

		if (unzOpenCurrentFile(a.handle) != UNZ_OK)
			throw archive_error{ "error opening file in archive"s };

		auto file_info = unz_file_info64{};
		unzGetCurrentFileInfo64(a.handle, &file_info, nullptr, 0, nullptr, 0, nullptr, 0);

		auto character_buffer = std::string(file_info.size_filename, '\0');
		unzGetCurrentFileInfo64(a.handle, nullptr, character_buffer.data(),
			integer_cast<uLong>(size(character_buffer)), nullptr, 0, nullptr, 0);

		a.file = character_buffer;
		return;
	}

	bool open_next_file(unarchive& a)
	{
		if (const auto next = unzGoToNextFile(a.handle); next == UNZ_END_OF_LIST_OF_FILE)
			return false;
		else if(next != UNZ_OK)
			throw archive_error{ "error opening file in archive"s };

		if (unzOpenCurrentFile(a.handle) != UNZ_OK)
			throw archive_error{ "error opening file in archive"s };

		auto file_info = unz_file_info64{};
		auto ret = unzGetCurrentFileInfo64(a.handle, &file_info, {}, {}, {}, {}, {}, {});

		if (ret < 0)
			throw archive_error{ "error opening file in archive"s }; 

		auto character_buffer = std::string(file_info.size_filename, '\0');
		unzGetCurrentFileInfo64(a.handle, nullptr, character_buffer.data(),
			integer_cast<uLong>(size(character_buffer)), nullptr, 0, nullptr, 0);

		a.file = character_buffer;
		return true;
	}

	void close_file(unarchive& a)
	{
		assert(a.handle);
		assert(!a.file.empty());
		a.file.clear();
		if (unzCloseCurrentFile(a.handle) == UNZ_CRCERROR)
			throw archive_error{ "CRC error reading file: " + a.file.generic_string() };
		return;
	}

	void close_archive(unarchive& f) noexcept
	{
		assert(f.handle);
		const auto r = unzClose(f.handle);
		if (r != UNZ_OK)
			log_error("Error while closing archive"sv);

		const auto iter = open_for_read.find(f.path.generic_string());
		assert(iter != end(open_for_read));
		open_for_read.erase(iter);
		f = {};

		return;
	}

	constexpr int case_sensitivity_auto = 0,//no sensitivity on windows
		case_sensitivity_sensitive = 1,		//case sensitivity everywhere
		case_sensitivity_none = 2; 			//no sensitivity on any platform

	bool file_exists(const unarchive& a, const std::filesystem::path& path) noexcept
	{
		assert(a.handle);
		const auto path_str = path.generic_string();
		const auto r = unzLocateFile(a.handle, path_str.c_str(), case_sensitivity_sensitive);
		return r == UNZ_OK;
	}

	toarchive create_archive(const std::filesystem::path& path)
	{
		auto path_str = path.generic_string(); 
		if (open_for_read.contains(path_str) || open_for_write.contains(path_str))
			throw files::file_error{ "Cannot open archive: " + path_str + ", for writing; it is already being used"s };

		auto a = toarchive{ path_str };
		if (!fs::exists(path))
		{
			a.handle = zipOpen64(path_str.c_str(), APPEND_STATUS_CREATE);
			if(a.handle)
				log_debug("Created new archive: "s + path_str);
		}
		else
		{
			const auto file = std::ofstream{ path }; // hold the file
			if (file)
			{
				if (fs::file_size(path) != 0)
				{
					try
					{
						fs::resize_file(path, 0);
					}
					catch (const fs::filesystem_error& e)
					{
						throw files::file_error{ e.what() };
					}
				}
			}
			else
				throw files::file_error{ "Cannot open file: "s + path.generic_string() };

			a.handle = zipOpen64(path_str.c_str(), APPEND_STATUS_CREATEAFTER);
			if(a.handle)
				log_debug("Overwriting archive: "s + path_str);
		}

		if (!a.handle)
			throw archive_error{ "Unable to create archive: "s + path.generic_string() };

		a.path = path;
		open_for_write.emplace(std::move(path_str));
		return a;
	}

	void open_file(toarchive& a, const std::filesystem::path& f)
	{
		const auto path_str = f.generic_string();
		
		auto ret = zipOpenNewFileInZip64(a.handle,
			path_str.c_str(), {}, {}, {}, {}, {}, {},
			Z_DEFLATED,
			Z_BEST_COMPRESSION,
			1);

		if (ret < 0)
			throw archive_error{ "Failed to create file in archive"s };

		a.file = f;
		return;
	}

	void close_file(toarchive& a) noexcept
	{
		assert(a.handle);
		assert(!a.file.empty());

		if (zipCloseFileInZip(a.handle) < 0)
			log_error("error writing file in archive: "s + a.file.generic_string());

		a.file.clear();
		return;
	}

	void close_archive(toarchive& f) noexcept
	{
		const auto r = zipClose(f.handle, {});
		if (r != ZIP_OK)
			log_error("tried to close archive before closing file"sv);

		open_for_write.erase(f.path.generic_string());
		f.handle = {};
		f.path.clear();
		return;
	}

	basic_in_compressed_filebuf<char>& basic_in_compressed_filebuf<char>::operator=(basic_in_compressed_filebuf<char>&& rhs) noexcept
	{
		std::swap(_get_area, rhs._get_area);
		std::swap(_device_buffer, rhs._device_buffer);
		std::swap(_file, rhs._file);
		std::swap(_zip_stream, rhs._zip_stream);

		// fix up the access ptrs
		auto ptr = rhs.gptr();
		auto end = rhs.egptr();
		auto ptr_offset = std::distance(rhs._get_area.data(), ptr);
		auto end_offset = std::distance(rhs._get_area.data(), end);
		auto beg = _get_area.data();
		setg(beg, beg + ptr_offset, beg + end_offset);
		rhs.setg(nullptr, nullptr, nullptr);

		using z_byte = Bytef;
		using z_uint = uInt;

		//// fixup the zip stream ptrs
		if (_zip_stream->z.next_in)
		{
			const auto in_distance = std::distance(rhs._get_area.data(), reinterpret_cast<char*>(_zip_stream->z.next_in));
			_zip_stream->z.next_in = reinterpret_cast<z_byte*>(_get_area.data() + in_distance);
		}

		if (_zip_stream->z.next_out)
		{
			const auto out_distance = std::distance(rhs._device_buffer.data(), reinterpret_cast<std::byte*>(_zip_stream->z.next_out));
			_zip_stream->z.next_out = reinterpret_cast<z_byte*>(_device_buffer.data() + out_distance);
		}
		return *this;
	}

	basic_in_compressed_filebuf<char>* basic_in_compressed_filebuf<char>::close() noexcept
	{
		if (_file)
		{
			inflateEnd(&_zip_stream->z);
			const auto ret = std::fclose(_file);
			_file = {};
			return ret == 0 ? this : nullptr;
		}
		return this;
	}

	basic_in_compressed_filebuf<char>::int_type basic_in_compressed_filebuf<char>::underflow()
	{
		auto ptr = gptr(), end = egptr();
		if (ptr != end)
			return traits_type::to_int_type(*ptr);

		if(end != _get_area.data() + size(_get_area))
			return traits_type::eof();

		return _fill_buffer();
	}

	basic_in_compressed_filebuf<char>::int_type basic_in_compressed_filebuf<char>::_fill_buffer()
	{
		// assume the get buffer is empty
		// 
		// fill the device buffer
		// feed into the get buffer
		// repeat until the get buffer is full

		//uint as defined by zlib
		using z_uint = uInt;
		using z_byte = Bytef;

		_zip_stream->z.next_out = reinterpret_cast<z_byte*>(_get_area.data());
		_zip_stream->z.avail_out = integer_cast<z_uint>(size(_get_area));

		while (_zip_stream->z.avail_out > 0)
		{
			// fill device buffer
			if (_zip_stream->z.avail_in < size(_device_buffer))
			{
				const auto buffer_end = _device_buffer.data() + size(_device_buffer);

				const auto next_in = std::move(reinterpret_cast<std::byte*>(_zip_stream->z.next_in),
					buffer_end, _device_buffer.data());
				const auto dist = std::distance(next_in, buffer_end);
				const auto read_amount = std::fread(next_in, sizeof(std::byte), dist, _file);
				const auto new_buffer_end = next_in + read_amount;
				_zip_stream->z.avail_in = integer_cast<z_uint>(std::distance(_device_buffer.data(),
					new_buffer_end));
				_zip_stream->z.next_in = reinterpret_cast<z_byte*>(_device_buffer.data());
			}

			const auto ret = ::inflate(&_zip_stream->z, Z_NO_FLUSH);
			if (ret == Z_STREAM_END)
			{
				auto beg = _get_area.data();
				auto end = _get_area.data() + (size(_get_area) - _zip_stream->z.avail_out);
				setg(beg, beg, end);
				return traits_type::to_int_type(*beg);
			}
			else if (ret != Z_OK)
			{
				auto msg = std::string{};
				if (_zip_stream->z.msg)
					msg = _zip_stream->z.msg;

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
		}

		auto beg = _get_area.data();
		auto end = _get_area.data() + size(_get_area);
		setg(beg, beg, end);
		return traits_type::to_int_type(*beg);
	}

	void basic_in_compressed_filebuf<char>::_seek_beg() noexcept
	{		
		rewind(_file);
		inflateEnd(&_zip_stream->z);
		_start_zlib();
		_fill_buffer();
		return;
	}

	bool basic_in_compressed_filebuf<char>::_start_zlib() noexcept
	{
		*_zip_stream = {};
		_zip_stream->z.zalloc = Z_NULL;
		_zip_stream->z.zfree = Z_NULL;
		_zip_stream->z.opaque = Z_NULL;
		_zip_stream->z.avail_in = 0;
		_zip_stream->z.next_in = reinterpret_cast<Bytef*>(_device_buffer.data() + size(_device_buffer));

		auto beg = _get_area.data();
		auto ptr = beg + size(_get_area);
		setg(beg, ptr, ptr);
		_pos = {};

		return inflateInit(&_zip_stream->z) == Z_OK;
	}

	basic_out_compressed_filebuf<char>& basic_out_compressed_filebuf<char>::operator=(basic_out_compressed_filebuf&& rhs) noexcept
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
		const auto in_distance = std::distance(rhs._put_area.data(), reinterpret_cast<char*>(_zip_stream->z.next_in));
		_zip_stream->z.next_in = reinterpret_cast<z_byte*>(_put_area.data() + in_distance);
		const auto out_distance = std::distance(rhs._device_buffer.data(), reinterpret_cast<std::byte*>(_zip_stream->z.next_out));
		_zip_stream->z.next_out = reinterpret_cast<z_byte*>(_device_buffer.data() + out_distance);

		return *this;
	}

	basic_out_compressed_filebuf<char>* basic_out_compressed_filebuf<char>::close()
	{
		if (_file)
		{
			_deflate_some(Z_FINISH);
			sync(); 
			deflateEnd(&_zip_stream->z);
			const auto fret = std::fclose(_file);
			_file = {};
			return fret == 0 ? this : nullptr;
		}
		return this;
	}

	basic_out_compressed_filebuf<char>::int_type basic_out_compressed_filebuf<char>::overflow(int_type i)
	{
		const auto ptr = pptr(), end = epptr();
		if (ptr && ptr < end)
			return !traits_type::eof();

		// this function depends on these types being used as generic byte ptr types
		static_assert(sizeof(char_type) == sizeof(Bytef));

		_deflate_some(Z_NO_FLUSH);
		if (i != traits_type::eof())
		{
			const auto beg = pbase();
			*beg = traits_type::to_char_type(i);
			setp(beg, beg + 1, end);
		}

		return !traits_type::eof();
	}

	basic_out_compressed_filebuf<char>::int_type basic_out_compressed_filebuf<char>::sync()
	{
		const auto beg = pbase(), ptr = pptr();
		if (beg != ptr)
			_deflate_some(Z_NO_FLUSH);

		if (_zip_stream->z.avail_out != size(_device_buffer))
			_empty_device_buffer();

		return {};
	}

	// deflates untill the put buffer is empty, only writes out to file as needed
	void basic_out_compressed_filebuf<char>::_deflate_some(int flush)
	{
		const auto beg = pbase(), ptr = pptr(), end = epptr();

		//uint as defined by zlib
		using z_uint = uInt;
		using z_byte = Bytef;
		_zip_stream->z.avail_in = integer_cast<z_uint>(std::distance(beg, ptr));
		_zip_stream->z.next_in = reinterpret_cast<z_byte*>(beg);

		auto cont = true;
		do
		{
			// check avail_out
			if (_zip_stream->z.avail_out == 0)
				_empty_device_buffer();

			const auto ret = deflate(&_zip_stream->z, flush);
			if (ret != Z_OK)
			{
				auto msg = std::string{};
				if (_zip_stream->z.msg)
					msg = _zip_stream->z.msg;

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
			cont = (ret == Z_OK && _zip_stream->z.avail_out == 0);
		} while (cont);

		setp(beg, end);
		return;
	}

	void basic_out_compressed_filebuf<char>::_empty_device_buffer()
	{
		if (_zip_stream->z.avail_out != size(_device_buffer))
		{
			using z_byte = Bytef;
			using z_uint = uInt;

			const auto device_end = reinterpret_cast<std::byte*>(_zip_stream->z.next_out);
			const auto write_amount = std::distance(_device_buffer.data(), device_end);
			const auto out = fwrite(_device_buffer.data(), sizeof(std::byte), write_amount, _file);
			if (out != integer_cast<std::size_t>(write_amount))
				throw files::file_error{ "write error" };
			const auto flush = fflush(_file);
			if (flush != int{})
				throw files::file_error{ "flush error" };

			_zip_stream->z.next_out = reinterpret_cast<z_byte*>(_device_buffer.data());
			_zip_stream->z.avail_out = integer_cast<z_uint>(size(_device_buffer));
		}
		return;
	}

	bool basic_out_compressed_filebuf<char>::_start_zlib() noexcept
	{
		using z_uint = uInt;
		using z_byte = Bytef;

		*_zip_stream = {};
		_zip_stream->z.zalloc = Z_NULL;
		_zip_stream->z.zfree = Z_NULL;
		_zip_stream->z.opaque = Z_NULL;
		_zip_stream->z.avail_in = 0;
		_zip_stream->z.next_in = reinterpret_cast<z_byte*>(_put_area.data());
		_zip_stream->z.avail_out = integer_cast<z_uint>(size(_device_buffer));
		_zip_stream->z.next_out = reinterpret_cast<z_byte*>(_device_buffer.data());

		return deflateInit(&_zip_stream->z, Z_BEST_COMPRESSION) == Z_OK;
	}

	void basic_in_archived_filebuf<char>::_seek_beg()
	{
		assert(is_archive_open());
		assert(is_open());
		unzCloseCurrentFile(_archive.handle);
		unzOpenCurrentFile(_archive.handle);
		_readsome();
		return;
	}

	// fill the input buffer
	basic_in_archived_filebuf<char>::int_type basic_in_archived_filebuf<char>::_readsome()
	{
		const auto ret = unzReadCurrentFile(_archive.handle, _get_area.data(), integer_cast<unsigned int>(size(_get_area)));

		using namespace std::string_literals;

		if(ret < 0)
			throw archive_error{ "error while reading compresed file"s };

		if (ret != 0)
		{
			const auto beg = _get_area.data();
			setg(beg, beg, beg + ret);
			return traits_type::to_int_type(*beg);
		}
		else
			return traits_type::eof();
	}

	std::size_t basic_in_archived_filebuf<char>::_pos() const noexcept
	{
		const auto ptr = gptr(), end = egptr();
		return unztell64(_archive.handle) - std::distance(ptr, end);
	}

	void basic_out_archived_filebuf<char>::_consume_buffer()
	{
		const auto beg = pbase(), ptr = pptr();
		auto file = _archive.file;
		if (beg != ptr)
		{
			auto ret = zipWriteInFileInZip(_archive.handle, _put_area.data(),
				integer_cast<unsigned int>(std::distance(beg, ptr)));
			if (ret < 0)
				throw archive_error{ "error writing archive"s };
		}
		setp(beg, beg + size(_put_area));
		return;
	}
}
