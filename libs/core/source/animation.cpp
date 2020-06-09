#include "hades/animation.hpp"

#include <cassert>

#include "hades/data.hpp"
#include "hades/logging.hpp"
#include "hades/parser.hpp"
#include "hades/texture.hpp"
#include "hades/timers.hpp"

namespace hades::resources
{
	static void load_animation(resource_type<std::vector<animation_frame>> &r, data::data_manager &d);

	animation::animation() noexcept : resource_type<std::vector<animation_frame>>(load_animation) {}

	static void parse_animation(unique_id mod, const data::parser_node &n, data::data_manager &d)
	{
		//animations:
		//	example-animation:
		//		duration: 1.0s
		//		texture: example-texture
		//		width: +int
		//      height: +int
		//		frames:
		//			- [x, y, flipx, flipy, d] // xpos ypos, relative duration
		//			- [x, y, flipx, flipy, d]
		//			the third parameter onward is optional

		using namespace std::string_view_literals;
		constexpr auto resource_type = "animation"sv;

		const auto animations = n.get_children();

		for (const auto &a : animations)
		{
			const auto name = a->to_string();
			const auto id = d.get_uid(name);

			auto anim = d.find_or_create<animation>(id, mod);

			using namespace data::parse_tools;
			anim->duration = get_scalar(*a, "duration"sv, anim->duration, duration_from_string);
	
			const auto tex_id = anim->tex ? anim->tex->id : unique_id::zero;
			const auto new_tex_id = get_unique(*a, "texture"sv, tex_id);

			if (new_tex_id != unique_id::zero)
				anim->tex = d.find_or_create<texture>(new_tex_id, mod);

			anim->width = get_scalar(*a, "width"sv, anim->width);
			anim->height = get_scalar(*a, "height"sv, anim->height);

			const auto frames_node = a->get_child("frames"sv);

			if (!frames_node)
				continue;

			const auto frames = frames_node->get_children();

			auto total_duration = 0.f;
			auto frame_list = std::vector<animation_frame>{};
			frame_list.reserve(frames.size());

			for (const auto& f : frames)
			{
				const auto frame_info = f->get_children();
				if (frame_info.empty())
					continue;

				const auto frame_size = size(frame_info);

				if (frame_size < 2)
				{
					const auto message = "Animation frame requires at least x and y components, in mod: " +
						to_string(mod) + ", for animation: " + name;
					LOGWARNING(message);
					continue;
				}
 
				auto frame = animation_frame{ 
					frame_info[0]->to_scalar<texture_size_t>(), 
					frame_info[1]->to_scalar<texture_size_t>()
				};

				if (frame_size > 2)
					frame.flip_x = frame_info[2]->to_scalar<bool>();
					
				if (frame_size > 3)
					frame.flip_y = frame_info[3]->to_scalar<bool>();				

				if(frame_size > 4)
					frame.duration = frame_info[4]->to_scalar<float>();

				if (frame_size > 5)
				{
					const auto message = "animation frame contains more that the expected data, ignoring excess, frame[x, y, flip_x, flip_y, rel_duration], in mod: " +
						to_string(mod) + ", for animation: " + name;
					LOGWARNING(message);
				}

				total_duration += frame.duration;

				frame_list.emplace_back(std::move(frame));
			}//for frames

			//normalise the frame durations
			assert(total_duration != 0.f);
			for (auto &f : frame_list)
				f.duration /= total_duration;

			std::swap(anim->value, frame_list);
		}//for animations
	}//parse animations

	static void load_animation(resource_type<std::vector<animation_frame>> &r, data::data_manager &d)
	{
		auto &a = dynamic_cast<animation&>(r);
		if (!a.tex->loaded)
			//data->get will lazy load texture
			d.get<texture>(a.tex->id);
	}
}

namespace hades::animation
{
	static animation_frame_data get_frame(const resources::animation &animation, float progress)
	{
		//NOTE: based on the FrameAnimation algorithm from Thor C++
		//https://github.com/Bromeon/Thor/blob/master/include/Thor/Animations/FrameAnimation.hpp

		if (progress > 1.f || progress < 0.f)
		{
			LOGWARNING("Animation progress parameter must be in the range (0, 1) was: " + to_string(progress) +
				"; while setting animation: " + to_string(animation.id));
		}

		//force lazy load if the texture hasn't been loaded yet.
		if (!animation.tex->loaded)
			hades::data::get<hades::resources::texture>(animation.tex->id);

		auto prog = std::clamp(progress, 0.f, 1.f);

		//calculate the progress to find the correct rect for this time
		for (auto &frame : animation.value)
		{
			prog -= frame.duration;

			// Must be <= and not <, to handle case (progress == frame.duration == 1) correctly
			if (prog <= 0.f)
			{
				const auto frame_x = frame.flip_x ? frame.x + animation.width : frame.x;
				const auto frame_y = frame.flip_y ? frame.y + animation.height : frame.y;
				const auto width = frame.flip_x ? -animation.width : animation.width;
				const auto height = frame.flip_y ? -animation.height : animation.height;
				return { frame_x, frame_y, width, height };
			}
		}

		LOGWARNING("Unable to find correct frame for animation " + to_string(animation.id) +
			"animation progress was: " + to_string(progress));
		return {};
	}

	animation_frame_data get_frame(const resources::animation &animation, time_point t)
	{
		const auto ratio = normalise_time(t, animation.duration);
		return get_frame(animation, ratio);
	}

	void apply(const resources::animation &animation, float progress, sf::Sprite &target)
	{
		const auto [x, y, w, h] = get_frame(animation, progress);
		target.setTexture(animation.tex->value);
		target.setTextureRect({ x, y , w, h });
	}

	void apply(const resources::animation &animation, time_point progress, sf::Sprite &target)
	{
		const auto ratio = normalise_time(progress, animation.duration);
		apply(animation, ratio, target);
	}
}

namespace hades
{
	poly_quad make_quad_colour(rect_float quad, colour c) noexcept
	{
		const auto col = sf::Color{ c.r, c.g, c.b, c.a };

		return poly_quad{
			//first triangle
			sf::Vertex{ {quad.x, quad.y}, col }, //top left
			sf::Vertex{ {quad.x + quad.width, quad.y}, col }, //top right
			sf::Vertex{ {quad.x, quad.y + quad.height}, col }, //bottom left
			//second triangle
			sf::Vertex{ { quad.x + quad.width, quad.y }, col }, //top right
			sf::Vertex{ { quad.x + quad.width, quad.y + quad.height }, col }, //bottom right
			sf::Vertex{ { quad.x, quad.y + quad.height }, col } //bottom left
		};
	}

	poly_quad make_quad_animation(vector_float p, const resources::animation &a, const animation::animation_frame_data &f) noexcept
	{
		return make_quad_animation(p, {static_cast<float>(a.width), static_cast<float>(a.height)}, f);
	}

	poly_quad make_quad_animation(vector_float pos, vector_float size, const animation::animation_frame_data& f) noexcept
	{
		return make_quad_animation({ pos, size }, { static_cast<float>(f.x), static_cast<float>(f.y),
			static_cast<float>(f.w), static_cast<float>(f.h) });
	}

	poly_quad make_quad_animation(rect_float quad, rect_float texture_quad) noexcept
	{
		const auto quad_right_x = quad.x + quad.width;
		const auto quad_bottom_y = quad.y + quad.height;
		const auto tex_right_x = texture_quad.x + texture_quad.width;
		const auto tex_bottom_y = texture_quad.y + texture_quad.height;

		return poly_quad{
			//first triangle
			sf::Vertex{ {quad.x, quad.y}, { texture_quad.x, texture_quad.y } }, //top left
			sf::Vertex{ { quad_right_x, quad.y }, { tex_right_x, texture_quad.y } }, //top right
			sf::Vertex{ { quad.x, quad_bottom_y }, { texture_quad.x, tex_bottom_y } }, //bottom left
			//second triangle
			sf::Vertex{ { quad_right_x, quad.y },{ tex_right_x, texture_quad.y } }, //top right
			sf::Vertex{ { quad_right_x, quad_bottom_y },  { tex_right_x, tex_bottom_y } }, //bottom right
			sf::Vertex{ { quad.x, quad_bottom_y },  { texture_quad.x, tex_bottom_y } } //bottom left
		};
	}

	void register_animation_resource(data::data_manager &d)
	{
		using namespace std::string_view_literals;
		//requires texture
		register_texture_resource(d);

		d.register_resource_type("animations"sv, resources::parse_animation);
	}
}