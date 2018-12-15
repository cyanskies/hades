#include "hades/tiled_sprite.hpp"

#include <array>

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/animation.hpp"

namespace hades
{

	void tiled_sprite::set_animation(const resources::animation *a, time_point t)
	{
		assert(a);
		_animation = a;

		vector_float new_frame{};
		std::tie(new_frame.x, new_frame.y) = animation::get_frame(*a, t);
		
		if (new_frame != _frame
			|| _animation != a)
		{
			new_frame = _frame;
			_generate_buffer();
		}
	}

	void tiled_sprite::set_size(vector_float s)
	{
		_size = s;

		_generate_buffer();
	}

	void tiled_sprite::draw(sf::RenderTarget &t, sf::RenderStates s) const
	{
		assert(_animation);
		s.texture = &_animation->tex->value;
		s.transform *= getTransform();

		t.draw(_vert_buffer, s);
	}

	void tiled_sprite::_generate_buffer()
	{
		//we want to generate a repeating tiled area covering <size>
		const auto x_count = _size.x / _animation->width;
		const auto y_count = _size.y / _animation->height;

		float x_wholef, y_wholef;

		const auto x_part = std::modf(x_count, &x_wholef);
		const auto y_part = std::modf(y_count, &y_wholef);

		const auto x_whole = static_cast<int32>(x_wholef);
		const auto y_whole = static_cast<int32>(y_wholef);

		const auto vertex_x = static_cast<std::size_t>(std::ceil(x_count));
		const auto vertex_y = static_cast<std::size_t>(std::ceil(y_count));

		const vector_float anim_size{ static_cast<float>(_animation->width),
									  static_cast<float>(_animation->height) };

		const auto[anim_x, anim_y] = _frame;

		std::vector<sf::Vertex> vertex{};
		vertex.reserve(vertex_x * vertex_y);

		for (auto y = 0u; y <= vertex_y; ++y)
		{
			for (auto x = 0u; x <= vertex_x; ++x)
			{
				const vector_float position{ x * anim_size.x, y * anim_size.y };
				auto size = anim_size;
				//for the final column the size may be truncated
				if (x == vertex_x)
					size.x *= x_part;

				if (y == vertex_y)
					size.y *= y_part;

				const auto quad = make_quad_animation({ position, size }, { {anim_x, anim_y}, size });
				std::copy(std::begin(quad), std::end(quad), std::back_inserter(vertex));
			}
		}

		_vert_buffer.set_verts(vertex);
	}
}
