#include "hades/sf_streams.hpp"

#include "hades/logging.hpp"
#include "hades/utility.hpp"

namespace hades
{
	constexpr auto errorval = -1;

	sf::Int64 sf_resource_stream::read(void* data, sf::Int64 size)
	{
		try
		{
			_stream.read(static_cast<irfstream::char_type*>(data), integer_cast<std::size_t>(size));
		}
		catch (const files::file_error& e)
		{
			LOGERROR(e.what());
			return errorval;
		}

		return _stream.gcount();
	}

	sf::Int64 sf_resource_stream::seek(sf::Int64 position)
	{
		try
		{
			_stream.seekg(position);
		}
		catch (const files::file_error& e)
		{
			LOGERROR(e.what());
			return errorval;
		}

		return tell();
	}

	sf::Int64 sf_resource_stream::tell()
	{
		try
		{
			return _stream.tellg();
		}
		catch (const files::file_error & e)
		{
			LOGERROR(e.what());
			return errorval;
		}
	}

	sf::Int64 sf_resource_stream::getSize()
	{
		return _size;
	}
}
