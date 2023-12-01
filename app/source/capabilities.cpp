#include "hades/capabilities.hpp"

#include "SFML/Graphics/VertexBuffer.hpp"
#include "SFML/Graphics/Shader.hpp"

namespace hades
{
	void test_capabilities()
	{
		if (!sf::VertexBuffer::isAvailable())
			throw capability_missing_error{ "Vertex Buffer support is required" };

		if (!sf::Shader::isAvailable())
			throw capability_missing_error{ "Shader support is required" };

		return;
	}
}