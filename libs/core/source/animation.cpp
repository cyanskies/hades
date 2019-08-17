#include "hades/animation.hpp"

#include <cassert>

#include "hades/data.hpp"
#include "hades/logging.hpp"
#include "hades/parser.hpp"
#include "hades/texture.hpp"
#include "hades/timers.hpp"

namespace hades::resources
{
	void load_animation(resource_type<std::vector<animation_frame>> &r, data::data_manager &d);

	animation::animation() : resource_type<std::vector<animation_frame>>(load_animation) {}

	void parse_animation(unique_id mod, const data::parser_node &n, data::data_manager &d)
	{
		//animations:
		//	example-animation:
		//		duration: 1.0s
		//		texture: example-texture
		//		width: +int
		//      height: +int
		//		frames:
		//			- [x, y, d] // xpos ypos, relative duration
		//			- [x, y, d]
		//			the third parameter is optional

		using namespace std::string_view_literals;
		constexpr auto resource_type = "animation"sv;

		const auto animations = n.get_children();

		for (const auto &a : animations)
		{
			const auto name = a->to_scalar<unique_id>([&d](std::string_view s) {return d.get_uid(s); });

			auto anim = d.find_or_create<animation>(name, mod);

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

			anim->value.clear();

			auto total_duration = 0.f;
			auto frame_list = std::vector<animation_frame>{};
			frame_list.reserve(frames.size());

			for (const auto &f : frames)
			{
				const auto frame_info = f->get_children();
				if (frame_info.empty())
					continue;

				if (frame_info.size() < 2 ||
					frame_info.size() > 3)
				{
					const auto message = "Animation frame malformed, in mod: " + 
						to_string(mod)+ ", for animation: " + to_string(name);
					LOGWARNING(message);
					continue;
				}

				const auto x = frame_info[0]->to_scalar<texture::size_type>();
				const auto y = frame_info[1]->to_scalar<texture::size_type>();

				auto frame = animation_frame{ x, y };

				if(frame_info.size() == 3)
					frame.duration = frame_info[2]->to_scalar<float>();

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

	void load_animation(resource_type<std::vector<animation_frame>> &r, data::data_manager &d)
	{
		auto &a = static_cast<animation&>(r);
		if (!a.tex->loaded)
			//data->get will lazy load texture
			d.get<texture>(a.tex->id);
	}
}

namespace hades::animation
{
	animation_frame get_frame(const resources::animation &animation, float progress)
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
				return std::make_tuple(static_cast<float>(frame.x), static_cast<float>(frame.y));
		}

		LOGWARNING("Unable to find correct frame for animation " + to_string(animation.id) +
			"animation progress was: " + to_string(progress));
		return { 0.f, 0.f };
	}

	animation_frame get_frame(const resources::animation &animation, time_point t)
	{
		const auto ratio = normalise_time(t, animation.duration);
		return get_frame(animation, ratio);
	}

	void apply(const resources::animation &animation, float progress, sf::Sprite &target)
	{
		const auto [x, y] = get_frame(animation, progress);
		target.setTexture(animation.tex->value);
		target.setTextureRect({ static_cast<int>(x), static_cast<int>(y) , animation.width, animation.height });
	}

	void apply(const resources::animation &animation, time_point progress, sf::Sprite &target)
	{
		const auto ratio = normalise_time(progress, animation.duration);
		apply(animation, ratio, target);
	}
}

namespace hades
{
	poly_quad make_quad_colour(rect_float quad, colour c)
	{
		const auto col = sf::Color{ c.r, c.g, c.b, c.a };

		return poly_quad{
			//first triange
			sf::Vertex{ {quad.x, quad.y}, col }, //top left
			sf::Vertex{ {quad.x + quad.width, quad.y}, col }, //top right
			sf::Vertex{ {quad.x, quad.y + quad.height}, col }, //bottom left
			//second triange
			sf::Vertex{ { quad.x + quad.width, quad.y }, col }, //top right
			sf::Vertex{ { quad.x + quad.width, quad.y + quad.height }, col }, //bottom right
			sf::Vertex{ { quad.x, quad.y + quad.height }, col } //bottom left
		};
	}

	poly_quad make_quad_animation(vector_float p, const resources::animation &a, const animation::animation_frame &f) noexcept
	{
		return make_quad_animation(p, {static_cast<float32>(a.width), static_cast<float32>(a.height)}, a, f);
	}

	poly_quad make_quad_animation(vector_float pos, vector_float size, const resources::animation& a, const animation::animation_frame& f) noexcept
	{
		return make_quad_animation({ pos, size }, { std::get<0>(f), std::get<1>(f),
			static_cast<float32>(a.width), static_cast<float32>(a.height) });
	}

	poly_quad make_quad_animation(rect_float quad, rect_float texture_quad) noexcept
	{
		const auto quad_right_x = quad.x + quad.width;
		const auto quad_bottom_y = quad.y + quad.height;
		const auto tex_right_x = texture_quad.x + texture_quad.width;
		const auto tex_bottom_y = texture_quad.y + texture_quad.height;

		return poly_quad{
			//first triange
			sf::Vertex{ {quad.x, quad.y}, { texture_quad.x, texture_quad.y } }, //top left
			sf::Vertex{ { quad_right_x, quad.y }, { tex_right_x, texture_quad.y } }, //top right
			sf::Vertex{ { quad.x, quad_bottom_y }, { texture_quad.x, tex_bottom_y } }, //bottom left
			//second triange
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