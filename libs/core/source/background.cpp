#include "hades/background.hpp"

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/animation.hpp"
#include "hades/data.hpp"
#include "hades/parser.hpp"

namespace hades::resources
{
	static void load_background(resource_type<background_t> &r, data::data_manager &d)
	{
		assert(dynamic_cast<background*>(&r));
		auto &b = static_cast<background&>(r);

		for (auto &l : b.layers)
		{
			assert(l.animation);
			if (!l.animation->loaded)
				//lazy load the animation
				d.get<animation>(l.animation->id);
		}
	}

	background::background() : resource_type{ load_background }
	{}

	static void parse_background(unique_id m, const data::parser_node &n, data::data_manager &d)
	{
		//backgrounds:
		//		backgound:
		//			colour: l
		//			layers:
		//				- [anim, {optional}parallax.x, parallax.y, {optional}offset.x, offset.y]

		const auto backgrounds = n.get_children();

		for (const auto &b : backgrounds)
		{
			const auto name = b->to_string();
			const auto id = d.get_uid(name);

			auto back = d.find_or_create<background>(id, m);
			if (!back)
				continue;

			using namespace std::string_view_literals;

			const auto colour = b->get_child("colour"sv);
			if (colour)
			{
				const auto bits = colour->to_sequence<uint8>();
				if (bits.size() > 0)
					back->colour.r = bits[0];
				if (bits.size() > 1)
					back->colour.g = bits[1];
				if (bits.size() > 2)
					back->colour.b = bits[2];
				if (bits.size() > 3)
					back->colour.a = bits[3];
			}

			const auto layers = b->get_child("layers"sv);
			if (!layers)
				continue;

			const auto layer_seq = layers->get_children();

			for (auto i = 0u; i < layer_seq.size(); ++i)
			{
				background::layer *layer = nullptr;

				if (back->layers.size() > i)
					layer = &back->layers[i];
				else
					layer = &back->layers.emplace_back();

				const auto &lay = layer_seq[i];

				assert(lay);
				if (!lay)
					continue;

				const auto &l = lay->get_children();

				if (l.empty())
				{
					const auto message = "Error parsing " + to_string(m)
						+ ", in background: " + name + ", unable to parse layer "
						+ to_string(i);
					LOGERROR(message);
					continue;
				}

				//get anim[required]
				const auto anim = l[0]->to_scalar<unique_id>();
				if (anim == unique_id::zero)
				{
					const auto message = "Error parsing " + to_string(m)
						+ ", in background: " + name + ", unable to parse animation id"
						+ " for layer: " + to_string(i);
					LOGERROR(message);
					continue;
				}

				const auto *animation = d.find_or_create<resources::animation>(anim, m);
				assert(animation);

				layer->animation = animation;

				//get parallax
				if (l.size() < 3)
					continue; //not enough values for a parallax

				const auto &par_x = l[1];
				const auto &par_y = l[2];

				assert(par_x && par_y);
				if (!(par_x && par_y))
				{
					const auto message = "Error parsing " + to_string(m) + "::"
						+ "background(" + name + "), parallax error in layer "
						+ to_string(i);
					LOGWARNING(message);
				}
				else
				{

					const auto parallax = vector_float{
						par_x->to_scalar<float>(),
						par_y->to_scalar<float>()
					};

					layer->parallax = parallax;
				}
				//get offset
				if (l.size() < 5)
					continue; //not enough values for an offset

				const auto &off_x = l[3];
				const auto &off_y = l[4];

				if (!(off_x && off_y))
				{
					const auto message = "Error parsing " + to_string(m) + "::"
						+ "background(" + name + "), offset error in layer "
						+ to_string(i);
					LOGWARNING(message);
				}
				else
				{

					const auto offset = vector_float{
						off_x->to_scalar<float>(),
						off_y->to_scalar<float>()
					};

					layer->offset = offset;
				}//for layers
			}
		}// for backgrounds
	}//parse_background
}

namespace hades
{
	void register_background_resource(data::data_manager &d)
	{
		using namespace std::string_view_literals;
		//requires animations
		register_animation_resource(d);

		d.register_resource_type("backgrounds"sv, resources::parse_background);
	}

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

	void background::set_colour(colour c) noexcept
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

	static void set_view(background::background_layer &b, rect_float v, vector_float s) noexcept
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

	void background::update(rect_float v) noexcept
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
