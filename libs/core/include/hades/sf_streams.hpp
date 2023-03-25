#ifndef HADES_SF_STREAMS_HPP
#define HADES_SF_STREAMS_HPP

#include "SFML/System/InputStream.hpp"

#include "hades/logging.hpp" // log_error
#include "hades/utility.hpp" // integer_cast

namespace hades
{
	// Wrap any stream which implements the cpp std stream interface
	// so that they can be used by any SFML functions that use streams.
	template<typename Stream>
	class sf_stream_wrapper final : public sf::InputStream
	{
	public:
		sf_stream_wrapper(Stream strm)
			: _stream{ std::move(strm) }
		{
			_stream.seekg({}, std::ios_base::end);
			_size = integer_cast<std::int64_t>(static_cast<std::streamoff>(_stream.tellg()));
			_stream.seekg({}, std::ios_base::beg);
			return;
		}

		template<typename ...Args>
		sf_stream_wrapper(Args&&... args)
			: sf_stream_wrapper{ Stream{ std::forward<Args>(args)... } }
		{}

		std::int64_t read(void* data, std::int64_t size) noexcept final override 
		{
			try
			{
                _stream.read(static_cast<Stream::char_type*>(data), integer_cast<std::streamsize>(size));
			}
			catch (const std::exception& e)
			{
				log_error(e.what());
				return errorval;
			}

			return _stream.gcount();
		}

		std::int64_t seek(std::int64_t position) noexcept final override
		{
			try
			{
				_stream.seekg(position);
			}
			catch (const std::exception& e)
			{
				log_error(e.what());
				return errorval;
			}

			return tell();
		}

		std::int64_t tell() noexcept final override
		{
			try
			{
				return _stream.tellg();
			}
			catch (const std::exception& e)
			{
				log_error(e.what());
				return errorval;
			}
		}

		std::int64_t getSize() noexcept final override
		{
			return _size;
		}

		const Stream& stream() const noexcept
		{
			return _stream;
		}

	private:
		static constexpr std::int64_t errorval = -1;
		Stream _stream;
		std::int64_t _size;
	};
}

#endif // !HADES_SF_STREAMS_HPP
