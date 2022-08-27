#ifndef HADES_SF_STREAMS_HPP
#define HADES_SF_STREAMS_HPP

#include "SFML/System/InputStream.hpp"

#include "hades/files.hpp"

namespace hades
{
	class sf_resource_stream : public sf::InputStream
	{
	public:
		sf_resource_stream(std::filesystem::path mod, std::filesystem::path file)
			: _stream{ mod, file }
		{
			_stream.seekg({}, std::ios_base::end);
			_size = integer_cast<sf::Int64>(static_cast<std::streamoff>(_stream.tellg()));
			_stream.seekg({}, std::ios_base::beg);
			return;
		}

		/*explicit sf_resource_stream(irfstream s)
			: _stream{ std::move(s) }
		{
			if (!_stream.is_open())
				throw files::file_not_open{ "Passed empty stream to sf_resource_stream" };
			return;
		}*/

		sf::Int64 read(void* data, sf::Int64 size) override;
		sf::Int64 seek(sf::Int64 position) override;
		sf::Int64 tell() override;
		sf::Int64 getSize() override;

	private:
		irfstream _stream;
		sf::Int64 _size;
	};
}

#endif // !HADES_SF_STREAMS_HPP
