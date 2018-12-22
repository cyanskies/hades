#include "hades/background.hpp"

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/animation.hpp"
#include "hades/data.hpp"

namespace hades
{
	static constexpr vector_float absolute_offset(vector_float o, vector_int a) noexcept
	{
		//TODO: handle offset > animation size
		return vector_float{
			o.x <= 0.f ? o.x : -(static_cast<float>(a.x) - o.x),
			o.y <= 0.f ? o.y : -(static_cast<float>(a.y) - o.y)
		} * -1.f;
	}

	background::background(vector_float size, const std::vector<layer> &layers, colour c)
		: _size(size)
	{
		set_size(size);

		set_colour(c);

		for (const auto &l : layers)
			add(l);
	}

	void background::set_colour(colour c)
	{
		_backdrop.setFillColor({ c.r, c.g, c.b, c.a });
	}

	void background::set_size(vector_float s)
	{
		_size = s;
		_backdrop.setSize({ s.x, s.y });
	}

	void background::add(layer l)
	{
		auto anim = data::get<resources::animation>(l.animation);
		assert(anim);
		_layers.emplace_back(background_layer{ anim, l.offset, l.parallax });
	}

	void background::clear() noexcept
	{
		_layers.clear();
	}

	static void set_animation(background::background_layer &b, time_point t, vector_float s)
	{
		b.sprite.set_animation(b.animation, t, tiled_sprite::dont_regen);
		const auto offset = absolute_offset(b.offset, { b.animation->width, b.animation->height });
		b.sprite.set_size({
			b.parallax.x > 0 ? std::max(s.x * b.parallax.x, s.x) : s.x,
			b.parallax.y > 0 ? std::max(s.x * b.parallax.y, s.y) : s.x, 
		});
	}

	void background::update(time_point t)
	{
		for (auto &l : _layers)
			set_animation(l, t, _size);
	}

	static void set_view(background::background_layer &b, rect_float v, vector_float s)
	{
		const auto view_pos = position(v);
		const auto view_size = size(v);

		const auto clamped_pos = vector_float{
			std::clamp(view_pos.x, 0.f, s.x),
			std::clamp(view_pos.y, 0.f, s.y)
		};

		const auto offset = absolute_offset(b.offset, { b.animation->width, b.animation->height });

		const auto offset_pos = clamped_pos - offset;

		const auto parallax_pos = vector_float{
			offset_pos.x * b.parallax.x,
			offset_pos.y * b.parallax.y
		};

		b.sprite.setPosition({ parallax_pos.x, parallax_pos.y });
	}

	void background::update(rect_float v)
	{
		for (auto &l : _layers)
			set_view(l, v, _size);
	}

	void background::update(time_point t, rect_float v)
	{
		for (auto &l : _layers)
		{
			set_animation(l, t, _size);
			set_view(l, v, _size);
		}
	}

	void background::draw(sf::RenderTarget &t, sf::RenderStates s) const
	{
		t.draw(_backdrop, s);
		for (const auto &l : _layers)
			t.draw(l.sprite);
	}
}
