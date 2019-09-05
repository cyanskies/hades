#include "hades/vertex_buffer.hpp"

#include <cassert>

#include "SFML/Graphics/RenderTarget.hpp"

static bool buffer_available()
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
		
		if (buffer_available()
			&& std::size(_verts) <= _buffer.getVertexCount())
		{
			_buffer.update(q.data(), std::size(q), std::size(_verts) - std::size(q));
		}

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

		if (buffer_available()
			&& offset + std::size(q) <= _buffer.getVertexCount())
		{
			_buffer.update(std::data(q), std::size(q), offset);
		}

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
		if (!buffer_available())
			return;

		const auto size = std::size(_verts);
		
		if (const auto old_size = _buffer.getVertexCount(),
			size = std::size(_verts);
			old_size == 0)
		{
			//how many quads the buffer should be able to hold
			constexpr auto starting_quad_capacity = 6u;
			constexpr auto starting_size = starting_quad_capacity * quad_vert_count;
			_buffer.create(std::max(starting_size, size));
		}
		else if (size > old_size)
		{
			constexpr auto growth_rate = 1.5f;
			const auto next_size = static_cast<std::size_t>(old_size * growth_rate);
			const auto remain = next_size % quad_vert_count;
			
			_buffer.create(next_size + remain);
			_buffer.update(_verts.data(), size, std::size_t{});
		}

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
		{
			//NOTE: transforms from s won't apply when drawing with _buffer
			//		for consistant results, we'll erase them from draws with
			//		_verts as well
			s.transform = sf::Transform{};
			t.draw(_verts.data(), std::size(_verts), _prim_type, s);
		}
		return;
	}
}
