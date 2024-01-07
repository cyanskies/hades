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
	class quad_buffer : public sf::Drawable
	{
	public:
		quad_buffer() noexcept = default;
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

		void draw(sf::RenderTarget&, std::size_t first_quad, std::size_t quad_count, const sf::RenderStates& states) const;

		// shink the vector and buffer size, to only the memory needed
		// good for static data
		void shrink_to_fit();

	protected:
		void draw(sf::RenderTarget&, const sf::RenderStates& = sf::RenderStates{}) const override;

	private:
		static constexpr auto _prim_type = sf::PrimitiveType::Triangles;
		sf::VertexBuffer _buffer{ _prim_type, sf::VertexBuffer::Usage::Stream };
		std::vector<sf::Vertex> _verts;
	};
}

#endif //!HADES_VERTEX_BUFFER_HPP
