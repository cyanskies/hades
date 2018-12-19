#include "hades/background.hpp"

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/animation.hpp"
#include "hades/data.hpp"

namespace hades
{
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

	void background::update(time_point t)
	{
		for (auto &l : _layers)
		{
			l.sprite.set_animation(l.animation, t);
			const auto size = vector_float{ 
				l.parallax.x == 0 ? _size.x : _size.x * l.parallax.x,
				l.parallax.y == 0 ? _size.y : _size.y * l.parallax.y 
			};

			l.sprite.set_size(size);
		}
	}

	void background::update(vector_float view_position)
	{
		const auto pos = vector::clamp(view_position, { 0.f, 0.f }, _size);

		for (auto &l : _layers)
		{
			const auto parallax_pos = vector_float{ pos.x * l.parallax.x, pos.y * l.parallax.y };
			const auto final_pos = parallax_pos + l.offset;
			l.sprite.setPosition({ final_pos.x, final_pos.y });
		}
	}

	void background::draw(sf::RenderTarget &t, sf::RenderStates s) const
	{
		t.draw(_backdrop, s);
		for (const auto &l : _layers)
			t.draw(l.sprite);
	}
}