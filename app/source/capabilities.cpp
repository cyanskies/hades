#include "hades/capabilities.hpp"

#include "SFML/Graphics/VertexBuffer.hpp"

namespace hades
{
	void test_capabilities()
	{
		if (!sf::VertexBuffer::isAvailable())
			throw capability_missing_error{ "Vertex Buffer support is required" };

		return;
	}
}