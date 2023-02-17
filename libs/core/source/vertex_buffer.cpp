#include "hades/vertex_buffer.hpp"

#include <cassert>

#include "SFML/System/Err.hpp"
#include "SFML/Graphics/RenderTarget.hpp"

using namespace std::string_literals;

namespace hades
{
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
		_verts.insert(end(_verts), begin(q), end(q));
		return;
	}

	poly_quad quad_buffer::get_quad(std::size_t offset) const noexcept
	{
		offset *= quad_vert_count;

		auto out = poly_quad{};
		assert(offset + std::size(out) <= std::size(_verts));

		auto data = &_verts[offset];
		for (auto& v : out)
			v = *data++;

		return out;
	}
	
	void quad_buffer::replace(const poly_quad& q, std::size_t offset) noexcept
	{
		offset *= quad_vert_count;
		assert(offset + std::size(q) <= std::size(_verts));

		auto data = &_verts[offset];
		for (const auto &v : q)
			*data++ = v;
		
		return;
	}
	
	void quad_buffer::swap(std::size_t offset1, std::size_t offset2) noexcept
	{
		if (offset1 == offset2)
			return;

		offset1 *= quad_vert_count;
		offset2 *= quad_vert_count;
		assert(offset1 + quad_vert_count <= std::size(_verts));
		assert(offset2 + quad_vert_count <= std::size(_verts));

		std::swap_ranges(&_verts[offset1], &_verts[offset1 + (quad_vert_count)],
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

	void quad_buffer::clear() noexcept
	{
		_verts.clear();
		return;
	}

	std::size_t quad_buffer::capacity() const noexcept
	{
		return _verts.capacity() / quad_vert_count;
	}

	void quad_buffer::reserve(std::size_t size)
	{
		_verts.reserve(size * quad_vert_count);
		if (size > (_buffer.getVertexCount() / quad_vert_count))
		{
			auto sb = std::stringbuf{};
			const auto prev = sf::err().rdbuf(&sb);
			if (_buffer.create(size * quad_vert_count))
			{
				if (!_buffer.update(_verts.data(), std::size(_verts), 0))
					LOGWARNING("Failed to update vertex buffer. "s + sb.str());
			}
			else
				LOGERROR("Unable to create vertex buffer. "s + sb.str());

            sf::err().rdbuf(prev);
		}

		return;
	}

	void quad_buffer::apply()
	{
		const auto size = std::size(_verts);
		
		if (size == 0)
			return;

		auto sb = std::stringbuf{};
		const auto prev = sf::err().rdbuf(&sb);
		auto success = true;
        if (const auto old_size = _buffer.getVertexCount();
			old_size == 0)
		{
			//how many quads the buffer should be able to hold
			constexpr auto starting_quad_capacity = std::size_t{ 6 };
			constexpr auto starting_size = starting_quad_capacity * quad_vert_count;
			success = _buffer.create(std::max(starting_size, size));
		}
		else if (size > old_size)
		{
			constexpr auto growth_rate = 2;
			const auto next_size = old_size * growth_rate;
			const auto remain = next_size % quad_vert_count;
			
			success = _buffer.create(std::max(next_size + remain, size));
		}

		if (!success)
			LOGERROR("Failed to create vertex buffer. "s + sb.str());
		sb.str({});

		if (!_buffer.update(_verts.data(), size, std::size_t{}))
			LOGWARNING("Failed to update vertex buffer. "s + sb.str());

        sf::err().rdbuf(prev);

		return;
	}

	void quad_buffer::shrink_to_fit()
	{
		_verts.shrink_to_fit();

		auto sb = std::stringbuf{};
		const auto prev = sf::err().rdbuf(&sb);
		if (_buffer.create(std::size(_verts)))
		{
			if (!_buffer.update(_verts.data()))
				LOGWARNING("Failed to update vertex buffer. "s + sb.str());
		}
		else
			LOGERROR("Failed to shrink vertex buffer "s + sb.str());

        sf::err().rdbuf(prev);

		return;
	}
	
	void quad_buffer::draw(sf::RenderTarget& t, const sf::RenderStates& s) const
	{
		t.draw(_verts.data(), std::size(_verts), _prim_type, s);
		return;
	}
}
