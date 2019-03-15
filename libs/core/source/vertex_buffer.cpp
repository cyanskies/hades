#include "hades/vertex_buffer.hpp"

#include <cassert>

#include "SFML/Graphics/RenderTarget.hpp"

namespace hades
{
	vertex_buffer::vertex_buffer()
	{
		if (sf::VertexBuffer::isAvailable())
			_verts.emplace<sf::VertexBuffer>();
		else
			_verts.emplace<sf::VertexArray>();
	}

	vertex_buffer::vertex_buffer(sf::PrimitiveType t)
	{
		if (sf::VertexBuffer::isAvailable())
			_verts.emplace<sf::VertexBuffer>(t);
		else
			_verts.emplace<sf::VertexArray>(t);
	}

	vertex_buffer::vertex_buffer(sf::VertexBuffer::Usage u)
	{
		if (sf::VertexBuffer::isAvailable())
			_verts.emplace<sf::VertexBuffer>(u);
		else
			_verts.emplace<sf::VertexArray>();
	}

	vertex_buffer::vertex_buffer(sf::PrimitiveType t, sf::VertexBuffer::Usage u)
	{
		if (sf::VertexBuffer::isAvailable())
			_verts.emplace<sf::VertexBuffer>(t, u);
		else
			_verts.emplace<sf::VertexArray>(t);
	}

	void vertex_buffer::set_type(sf::PrimitiveType t)
	{
		std::visit([t](auto &&v) {
			v.setPrimitiveType(t);
		}, _verts);
	}

	void vertex_buffer::set_usage(sf::VertexBuffer::Usage u)
	{
		std::visit([u](auto &&v) {
			using T = std::decay_t<decltype(v)>;
			if constexpr (std::is_same_v<T, sf::VertexBuffer>)
				v.setUsage(u);
		}, _verts);
	}

	void vertex_buffer::set_verts(const std::vector<sf::Vertex> &vertex)
	{
		std::visit([&vertex](auto &&v) {
			using T = std::decay_t<decltype(v)>;
			const auto count = v.getVertexCount();
			const auto vertex_count = vertex.size();
			if constexpr (std::is_same_v<T, sf::VertexBuffer>)
			{
				if (count <= vertex_count)
				{
					if (count == 0u)
						v.create(vertex_count);

					v.update(std::data(vertex), vertex_count, 0u);
				}
				else
				{
					v.create(vertex_count);
					v.update(vertex.data());
				}
			}
			else // VertexArray
			{
				if (count != vertex_count)
					v.resize(vertex_count);

				for (auto i = 0u; i < vertex_count; ++i)
				{
					assert(i < count);
					v[i] = vertex[i];
				}
			}
		}, _verts);
	}

	void vertex_buffer::draw(sf::RenderTarget &t, sf::RenderStates s) const
	{
		std::visit([&t, &s](auto &&v) {
			t.draw(v, s);
		}, _verts);
	}
}
