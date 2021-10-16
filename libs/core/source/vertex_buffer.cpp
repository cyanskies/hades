#include "hades/vertex_buffer.hpp"

#include <cassert>

#include "SFML/Graphics/RenderTarget.hpp"

static bool buffer_available() noexcept
{
	static const auto avail = sf::VertexBuffer::isAvailable();
	return avail;
}

namespace hades
{
	vertex_buffer::vertex_buffer()
	{
		if (sf::VertexBuffer::isAvailable())
			_verts.emplace<sf::VertexBuffer>();
	}

	vertex_buffer::vertex_buffer(sf::PrimitiveType t)
	{
		if (sf::VertexBuffer::isAvailable())
			_verts.emplace<sf::VertexBuffer>(t);
		else
			_verts.emplace<vert_array>(vert_array{ {}, t });
	}

	vertex_buffer::vertex_buffer(sf::VertexBuffer::Usage u)
	{
		if (sf::VertexBuffer::isAvailable())
			_verts.emplace<sf::VertexBuffer>(u);
	}

	vertex_buffer::vertex_buffer(sf::PrimitiveType t, sf::VertexBuffer::Usage u)
	{
		if (sf::VertexBuffer::isAvailable())
			_verts.emplace<sf::VertexBuffer>(t, u);
		else
			_verts.emplace<vert_array>(vert_array{ {}, t });
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
				v.vertex = vertex;
			}
		}, _verts);
	}

	void vertex_buffer::draw(sf::RenderTarget &t, sf::RenderStates s) const
	{
		std::visit([&t, &s](auto &&v) {
			if constexpr (std::is_same_v<std::decay_t<decltype(v)>, sf::VertexBuffer>)
			{
				t.draw(v, s);
			}
			else
				t.draw(v.vertex.data(), v.getVertexCount(), v.primative, s);
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

		std::swap_ranges(&_verts[offset1], &_verts[offset1 + (quad_vert_count - 1u)],
			&_verts[offset2]);

		return;
	}

	void quad_buffer::pop_back() noexcept
	{
		//remove the last 6 vertexes'
		auto target = end(_verts);
		std::advance(target, -6);
		_verts.erase(target, end(_verts));
		return;
	}
	
	void quad_buffer::apply()
	{
		if (!buffer_available())
			return;

		const auto size = std::size(_verts);
		
		if (const auto old_size = _buffer.getVertexCount(),
			vert_size = std::size(_verts);
			old_size == 0)
		{
			//how many quads the buffer should be able to hold
			constexpr auto starting_quad_capacity = std::size_t{ 6 };
			constexpr auto starting_size = starting_quad_capacity * quad_vert_count;
			_buffer.create(std::max(starting_size, vert_size));
		}
		else if (size > old_size)
		{
			constexpr auto growth_rate = 2;
			const auto next_size = old_size * growth_rate;
			const auto remain = next_size % quad_vert_count;
			
			_buffer.create(next_size + remain);
		}

		_buffer.update(_verts.data(), size, std::size_t{});

		return;
	}

	void quad_buffer::shrink_to_fit()
	{
		_verts.shrink_to_fit();

		if (buffer_available())
		{
			_buffer.create(std::size(_verts));
			_buffer.update(_verts.data());
		}

		return;
	}
	
	void quad_buffer::draw(sf::RenderTarget& t, sf::RenderStates s) const
	{
		if (buffer_available())
			t.draw(_buffer, std::size_t{}, std::size(_verts), s);
		else
			t.draw(_verts.data(), std::size(_verts), _prim_type, s);
		return;
	}
}
