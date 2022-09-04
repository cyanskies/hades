#ifndef HADES_SF_STREAMS_HPP
#define HADES_SF_STREAMS_HPP

#include "SFML/System/InputStream.hpp"

#include "hades/files.hpp"
#include "hades/logging.hpp"

namespace hades
{
	template<typename Stream>
	class sf_stream_wrapper final : public sf::InputStream
	{
	public:
		template<typename ...Args>
		sf_stream_wrapper(Args&&... args)
			: _stream{ std::forward<Args>(args)... }
		{
			_stream.seekg({}, std::ios_base::end);
			_size = integer_cast<sf::Int64>(static_cast<std::streamoff>(_stream.tellg()));
			_stream.seekg({}, std::ios_base::beg);
			return;
		}

		sf::Int64 read(void* data, sf::Int64 size) noexcept final override 
		{
			try
			{
				_stream.read(static_cast<irfstream::char_type*>(data), integer_cast<std::size_t>(size));
			}
			catch (const std::exception& e)
			{
				log_error(e.what());
				return errorval;
			}

			return _stream.gcount();
		}

		sf::Int64 seek(sf::Int64 position) noexcept final override
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

		sf::Int64 tell() noexcept final override
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

		sf::Int64 getSize() noexcept final override
		{
			return _size;
		}

		const Stream& stream() const noexcept
		{
			return _stream;
		}

	private:
		static constexpr sf::Int64 errorval = -1;
		Stream _stream;
		sf::Int64 _size;
	};

	using sf_resource_stream = sf_stream_wrapper<irfstream>;
}

#endif // !HADES_SF_STREAMS_HPP
