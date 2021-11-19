#include "hades/tiled_sprite.hpp"

#include <array>

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/animation.hpp"
#include "hades/texture.hpp"

namespace hades
{

	void tiled_sprite::set_animation(const resources::animation *a, time_point t)
	{
		assert(a);

		const auto old_frame = _frame;
		const auto old_anim = _animation;

		set_animation(a, t, dont_regen);

		if (old_frame != _frame
			|| old_anim != a)
		{
			_generate_buffer();
		}
	}

	void tiled_sprite::set_animation(const resources::animation *a, time_point t, dont_regen_t) noexcept
	{
		assert(a);
		_animation = a;
		_frame = animation::get_frame(*a, t);
		return;
	}

	void tiled_sprite::set_size(vector_float s)
	{
		set_size(s, dont_regen);

		_generate_buffer();
	}

	void tiled_sprite::set_size(vector_float s, dont_regen_t) noexcept
	{
		_size = s;
	}

	resources::animation_frame tiled_sprite::get_current_frame() const noexcept
	{
		return _frame;
	}

	void tiled_sprite::draw(sf::RenderTarget &t, sf::RenderStates s) const
	{
		assert(_animation);
		const auto tex = resources::animation_functions::get_texture(*_animation);
		assert(tex);
		s.texture = &resources::texture_functions::get_sf_texture(tex);
		s.transform *= getTransform();

		t.draw(_vert_buffer, s);
	}

	void tiled_sprite::_generate_buffer()
	{
		//we want to generate a repeating tiled area covering <size>
		const auto anim_size = vector_float{
			std::abs(_frame.w * _frame.scale_w),
			std::abs(_frame.h * _frame.scale_h)
		};

		const auto x_count = _size.x / anim_size.x;
		const auto y_count = _size.y / anim_size.y;

		float x_wholef, y_wholef;

		const auto x_part = std::modf(x_count, &x_wholef);
		const auto y_part = std::modf(y_count, &y_wholef);

		assert(x_wholef >= 0.f);
		assert(y_wholef >= 0.f);

		const auto vertex_x = static_cast<uint32>(x_wholef);
		const auto vertex_y = static_cast<uint32>(y_wholef);

		const auto tex_pos = vector_float{ _frame.x, _frame.y };
		const auto tex_size = vector_float{ _frame.w, _frame.h };

		std::vector<sf::Vertex> vertex{};
		vertex.reserve(vertex_x * vertex_y);

		for (auto y = 0u; y <= vertex_y; ++y)
		{
			for (auto x = 0u; x <= vertex_x; ++x)
			{
				const vector_float position{ x * anim_size.x, y * anim_size.y };
				auto size = tex_size;
				//for the final column the size may be truncated
				if (x == vertex_x)
					size.x *= x_part;

				if (y == vertex_y)
					size.y *= y_part;

				const auto quad = make_quad_animation({ position, size }, { tex_pos, size });
				vertex.insert(std::end(vertex), std::begin(quad), std::end(quad));
			}
		}

		_vert_buffer.set_verts(vertex);
	}
}
