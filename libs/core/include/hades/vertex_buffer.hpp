#ifndef HADES_VERTEX_BUFFER_HPP
#define HADES_VERTEX_BUFFER_HPP

#include <variant>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Vertex.hpp"
#include "SFML/Graphics/VertexArray.hpp"
#include "SFML/Graphics/VertexBuffer.hpp"

namespace hades
{
	//vertex buffer class, with fallback to array if the buffer is not available

	class vertex_buffer : public sf::Drawable
	{
	public:
		vertex_buffer();
		vertex_buffer(sf::PrimitiveType);
		vertex_buffer(sf::VertexBuffer::Usage);
		vertex_buffer(sf::PrimitiveType, sf::VertexBuffer::Usage);

		vertex_buffer(const vertex_buffer&) = default;
		vertex_buffer &operator=(const vertex_buffer&) = default;

		~vertex_buffer() noexcept = default;

		void set_type(sf::PrimitiveType);
		void set_usage(sf::VertexBuffer::Usage);
		void set_verts(const std::vector<sf::Vertex>&);

		void draw(sf::RenderTarget&, sf::RenderStates) const override;

	private:
		std::variant<sf::VertexArray, sf::VertexBuffer> _verts;
	};
}

#endif //!HADES_VERTEX_BUFFER_HPP