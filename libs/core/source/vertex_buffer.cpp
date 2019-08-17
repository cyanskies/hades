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

	quad_buffer::quad_buffer(sf::VertexBuffer::Usage u) noexcept : _buffer{_prim_type, u}
	{
		return;
	}

	quad_buffer::quad_buffer(const quad_buffer& rhs) : _verts{ rhs._verts }, _buffer{rhs._buffer}
	{
		return;
	}

	quad_buffer& quad_buffer::operator=(const quad_buffer& rhs)
	{
		_verts = rhs._verts;
		_buffer = rhs._buffer;
		return *this;
	}

	quad_buffer::quad_buffer(quad_buffer &&rhs) noexcept : _verts{std::move(rhs._verts)}
	{
		std::swap(_buffer, rhs._buffer);
		return;
	}

	quad_buffer& quad_buffer::operator=(quad_buffer &&rhs) noexcept
	{
		std::swap(_verts, rhs._verts);
		std::swap(_buffer, rhs._buffer);
		return *this;
	}

	std::size_t quad_buffer::size() const noexcept
	{
		return std::size(_verts) / quad_vert_count;
	}

	void quad_buffer::append(const poly_quad& q) 
	{
		for (const auto& v : q)
			_verts.emplace_back(v);
		return;
	}

	poly_quad quad_buffer::get_quad(std::size_t offset) noexcept
	{
		offset *= quad_vert_count;

		auto out = poly_quad{};
		assert(offset + std::size(out) <= std::size(_verts));

		for (auto i = std::size_t{}; i < std::size(out); ++i)
			out[i] = _verts[i];

		return out;
	}
	
	void quad_buffer::replace(const poly_quad& q, std::size_t offset) noexcept
	{
		offset *= quad_vert_count;
		assert(offset + std::size(q) <= std::size(_verts));

		for (auto i = std::size_t{}; i < std::size(q); ++i)
			_verts[i + offset] = q[i];

		return;
	}
	
	void quad_buffer::swap(std::size_t offset1, std::size_t offset2) noexcept
	{
		offset1 *= quad_vert_count;
		offset2 *= quad_vert_count;
		assert(offset1 + quad_vert_count <= std::size(_verts));
		assert(offset2 + quad_vert_count <= std::size(_verts));

		for (auto i = std::size_t{}; i < quad_vert_count; ++i)
			std::swap(_verts[i + offset1], _verts[i + offset2]);
		return;
	}

	void quad_buffer::pop_back() noexcept
	{
		//remove the last 6 vertexes'
		_verts.pop_back();
		_verts.pop_back();
		_verts.pop_back();
		_verts.pop_back();
		_verts.pop_back();
		_verts.pop_back();
	}
	
	void quad_buffer::apply()
	{
		if (!sf::VertexBuffer::isAvailable())
			return;

		const auto size = std::size(_verts);

		if (size != _buffer.getVertexCount())
			_buffer.create(size);

		_buffer.update(_verts.data());
		return;
	}
	
	void quad_buffer::draw(sf::RenderTarget& t, sf::RenderStates s) const
	{
		if (sf::VertexBuffer::isAvailable())
			t.draw(_buffer, s);
		else
			t.draw(_verts.data(), std::size(_verts), _prim_type, s);
		return;
	}
}
