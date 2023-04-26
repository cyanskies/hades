#ifndef HADES_DEFLATE_HPP
#define HADES_DEFlATE_HPP

#include <span>
#include <type_traits>
#include <vector>

#define ZLIB_CONST
#include "zlib.h"
#undef zlib_version

#include "hades/archive.hpp"

// NOTE: be careful including this header, it leaks zlib into the global namespace

namespace hades::zip
{
	template<typename Ty>
		requires std::is_trivially_copy_constructible_v<Ty>
	std::vector<std::byte> deflate(std::span<Ty> stream, bool text = false)
	{
		::z_stream deflate_stream;
		deflate_stream.zalloc = Z_NULL;
		deflate_stream.zfree = Z_NULL;
		deflate_stream.opaque = Z_NULL;
		deflate_stream.data_type = text ? Z_TEXT : Z_BINARY;

		//uint as defined by zlib
		using z_uint = uInt;

		deflate_stream.avail_in =
			integer_cast<z_uint>(std::size(stream) * sizeof(Ty)); // size of input, string + terminator
		deflate_stream.next_in = reinterpret_cast<const Bytef*>(stream.data()); // input char array

		// the actual compression work.
		auto ret = deflateInit(&deflate_stream, Z_BEST_COMPRESSION);
		if (ret != Z_OK)
			throw archive_error{ "failed to initialise zlib deflate" };

		//unsigned long as defined by zlib;
		using z_ulong = uLong;

		//get the size
		const auto size = deflateBound(&deflate_stream, deflate_stream.avail_in);

		auto out = std::vector<std::byte>(size);
		deflate_stream.avail_out = integer_cast<z_uint>(std::size(out)); // size of output
		deflate_stream.next_out = reinterpret_cast<Bytef*>(out.data()); // output char array

		ret = deflate(&deflate_stream, Z_FINISH);
		if (ret != Z_STREAM_END)
			throw archive_error{ "failed to deflate" };
		assert(ret == Z_STREAM_END);
		ret = deflateEnd(&deflate_stream);
		if (ret != Z_OK)
			throw archive_error{ "failed to finalise zlib deflate" };

		// remove unused buffer space
		out.resize(integer_cast<std::size_t>(deflate_stream.next_out - reinterpret_cast<Bytef*>(out.data())));
		return out;
	}

	template<typename Ty>
		requires std::is_default_constructible_v<Ty>&& std::is_trivially_copyable_v<Ty>
	std::vector<Ty> inflate(std::span<const std::byte> stream, std::size_t uncompressed_size = {})
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
		infstream.next_in = reinterpret_cast<const unsigned char*>(stream.data()); // input char array

		auto ret = inflateInit(&infstream);
		if (ret != Z_OK)
			throw archive_error{ "failed to initialise zlib inflate" };

		auto out = std::vector<Ty>(uncompressed_size == 0 ? std::size(stream) : uncompressed_size);

		auto begin = out.data();
		infstream.next_out = reinterpret_cast<unsigned char*>(begin);
		auto end = reinterpret_cast<unsigned char*>(begin + size(out));
		infstream.avail_out = integer_cast<z_uint>(end - infstream.next_out);

		while (true)
		{
			ret = inflate(&infstream, uncompressed_size == 0 ? Z_NO_FLUSH : Z_FINISH);
			if (ret == Z_STREAM_END)
				break;
			else if (ret != Z_OK)
				throw archive_error{ "failed to inflate" };

			const auto out_pos = reinterpret_cast<Ty*>(infstream.next_out) - begin;
			out.resize(size(out) * 2);
			begin = out.data();

			infstream.next_out = reinterpret_cast<unsigned char*>(begin + out_pos);
			end = reinterpret_cast<unsigned char*>(begin + size(out));
			infstream.avail_out = integer_cast<z_uint>(end - infstream.next_out);

			/*if (infstream.avail_out != 0)
				cont = false;*/
		}

		out.resize(integer_cast<std::size_t>(infstream.next_out - reinterpret_cast<Bytef*>(out.data())) / sizeof(Ty));

		ret = inflateEnd(&infstream);
		if (ret != Z_OK)
			throw archive_error{ "failed to finalise zlib inflate" };

		return out;
	}
}

#endif // !HADES_DEFlATE_HPP