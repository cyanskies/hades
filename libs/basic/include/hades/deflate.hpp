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
	std::vector<std::byte> deflate(std::span<Ty> stream)
	{
		::z_stream deflate_stream;
		deflate_stream.zalloc = Z_NULL;
		deflate_stream.zfree = Z_NULL;
		deflate_stream.opaque = Z_NULL;

		deflate_stream.avail_in =
			integer_cast<uInt>(std::size(stream) * sizeof(Ty)); // size of input, string + terminator
		deflate_stream.next_in = reinterpret_cast<const Bytef*>(stream.data()); // input char array

		// the actual compression work.
		auto ret = deflateInit(&deflate_stream, Z_BEST_COMPRESSION);
		if (ret != Z_OK)
			throw archive_error{ deflate_stream.msg };

		//get the size
		const auto size = deflateBound(&deflate_stream, deflate_stream.avail_in);

		auto out = std::vector<std::byte>(size);
		deflate_stream.avail_out = integer_cast<uInt>(std::size(out)); // size of output
		deflate_stream.next_out = reinterpret_cast<Bytef*>(out.data()); // output char array

		ret = deflate(&deflate_stream, Z_FINISH);
		if (ret != Z_STREAM_END)
			throw archive_error{ deflate_stream.msg };
		assert(ret == Z_STREAM_END);
		ret = deflateEnd(&deflate_stream);
		if (ret != Z_OK)
			throw archive_error{ deflate_stream.msg };

		// remove unused buffer space
		out.resize(integer_cast<std::size_t>(deflate_stream.next_out - reinterpret_cast<Bytef*>(out.data())));
		return out;
	}

	template<typename Cont>
		requires requires (const Cont& cont) {
		std::span{ cont };
	}
	std::vector<std::byte> deflate(const Cont& cont)
	{
		return deflate(std::span{ cont });
	}

	template<typename Ty>
		requires std::is_default_constructible_v<Ty>&& std::is_trivially_copyable_v<Ty>
	std::vector<Ty> inflate(std::span<const std::byte> stream, std::size_t uncompressed_size = {})
	{
		::z_stream infstream;
		infstream.zalloc = Z_NULL;
		infstream.zfree = Z_NULL;
		infstream.opaque = Z_NULL;

		// setup "b" as the input and "c" as the compressed output
		infstream.avail_in =
			integer_cast<uInt>(std::size(stream)); // size of input
		infstream.next_in = reinterpret_cast<const Bytef*>(stream.data()); // input char array

		auto ret = inflateInit(&infstream);
		if (ret != Z_OK)
			throw archive_error{ infstream.msg };

		auto out = std::vector<Ty>(uncompressed_size == 0 ? std::size(stream) : uncompressed_size);

		auto begin = out.data();
		infstream.next_out = reinterpret_cast<Bytef*>(begin);
		auto end = reinterpret_cast<Bytef*>(begin + size(out));
		infstream.avail_out = integer_cast<uInt>(end - infstream.next_out);

		while (true)
		{
			ret = inflate(&infstream, uncompressed_size == 0 ? Z_NO_FLUSH : Z_FINISH);
			if (ret == Z_STREAM_END)
				break;
			else if (ret < Z_OK)
				throw archive_error{ infstream.msg };

			const auto out_pos = reinterpret_cast<Ty*>(infstream.next_out) - begin;
			out.resize(size(out) * 2);
			begin = out.data();

			infstream.next_out = reinterpret_cast<Bytef*>(begin + out_pos);
			end = reinterpret_cast<Bytef*>(begin + size(out));
			infstream.avail_out = integer_cast<uInt>(end - infstream.next_out);
		}

		out.resize(integer_cast<std::size_t>(infstream.next_out - reinterpret_cast<Bytef*>(out.data())) / sizeof(Ty));

		ret = inflateEnd(&infstream);
		if (ret != Z_OK)
			throw archive_error{ infstream.msg };

		return out;
	}

	template<typename Ty, typename Cont>
		requires requires (const Cont& cont) {
		std::is_default_constructible_v<Ty> && std::is_trivially_copyable_v<Ty>;
		std::span<const std::byte>{ cont };
	}
	std::vector<Ty> inflate(const Cont &stream, std::size_t uncompressed_size = {})
	{
		return inflate<Ty>(std::span{ stream }, uncompressed_size);
	}
}

#endif // !HADES_DEFlATE_HPP