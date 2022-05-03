#ifndef HADES_VERTEX_BUFFER_HPP
#define HADES_VERTEX_BUFFER_HPP

#include <variant>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/Vertex.hpp"
#include "SFML/Graphics/VertexArray.hpp"
#include "SFML/Graphics/VertexBuffer.hpp"

#include "hades/animation.hpp"

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

		void draw(sf::RenderTarget&, const sf::RenderStates&) const override;

	private:
		struct vert_array
		{
			std::vector<sf::Vertex> vertex;
			sf::PrimitiveType primative = sf::PrimitiveType::Points;

			std::size_t getVertexCount() const noexcept
			{
				return size(vertex);
			}

			void setPrimitiveType(sf::PrimitiveType p) noexcept
			{
				primative = p;
				return;
			}
		};

		std::variant<vert_array, sf::VertexBuffer> _verts;
	};

	class quad_buffer : public sf::Drawable
	{
	public:
		quad_buffer() = default;
		quad_buffer(sf::VertexBuffer::Usage) noexcept;

		quad_buffer(const quad_buffer&);
		quad_buffer& operator=(const quad_buffer&);

		quad_buffer(quad_buffer&&) noexcept;
		quad_buffer& operator=(quad_buffer&&) noexcept;

		~quad_buffer() noexcept = default;

		//returns the number of quads
		std::size_t size() const noexcept;

		void append(const poly_quad&);
		poly_quad get_quad(std::size_t) const noexcept;
		void replace(const poly_quad&, std::size_t) noexcept;
		void swap(std::size_t, std::size_t) noexcept;
		void pop_back() noexcept;
		void clear() noexcept;

		//reserve space in the buffer
		std::size_t capacity() const noexcept;
		void reserve(std::size_t);
		void apply();
		// shink the vector and buffer size, to only the memory needed
		// good for static data
		void shrink_to_fit();

		void draw(sf::RenderTarget&, const sf::RenderStates& = sf::RenderStates{}) const override;

	private:
		static constexpr auto _prim_type = sf::PrimitiveType::Triangles;
		std::vector<sf::Vertex> _verts;
		sf::VertexBuffer _buffer{ _prim_type, sf::VertexBuffer::Usage::Stream };
	};
}

#endif //!HADES_VERTEX_BUFFER_HPP