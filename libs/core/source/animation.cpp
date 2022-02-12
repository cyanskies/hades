#include "hades/animation.hpp"

#include <cassert>

#include "hades/data.hpp"
#include "hades/logging.hpp"
#include "hades/parser.hpp"
#include "hades/sf_color.hpp"
#include "hades/texture.hpp"
#include "hades/timers.hpp"

namespace hades::resources
{
	static void load_animation(resource_type<std::vector<animation_frame>>& r, data::data_manager& d);
	//TODO: add field for fragment shaders
	struct animation : public resource_type<std::vector<animation_frame>>
	{
		animation() noexcept : resource_type<std::vector<animation_frame>>(load_animation)
		{}

		const texture* tex = nullptr;
		time_duration duration = time_duration::zero();
		//const shader *anim_shader = nullptr;
	};
}

namespace hades::resources::animation_functions
{
	const animation* get_resource(unique_id id)
	{
		return data::get<animation>(id);
	}

	const animation* get_resource(data::data_manager& d, unique_id id)
	{
		return d.get<animation>(id);
	}

	try_get_return try_get(unique_id id) noexcept
	{
		return data::try_get<animation>(id);
	}

	const animation* find_or_create(data::data_manager& d, unique_id id, unique_id mod)
	{
		return d.find_or_create<animation>(id, mod);
	}

	std::vector<const animation*> find_or_create(data::data_manager& d, const std::vector<unique_id>& ids, unique_id mod)
	{
		return d.find_or_create<const animation>(ids, mod);
	}

	bool is_loaded(const animation& a) noexcept
	{
		return a.loaded;
	}

	unique_id get_id(const animation& a) noexcept
	{
		return a.id;
	}

	std::vector<unique_id> get_id(const std::vector<const animation*>& a) noexcept
	{
		auto ret = std::vector<unique_id>{};
		ret.reserve(size(a));
		std::transform(begin(a), end(a), back_inserter(ret), [](const animation* a) noexcept {
			assert(a);
			return a->id;
		});
		return ret;
	}

	const texture* get_texture(const animation& a) noexcept
	{
		return a.tex;
	}

	time_duration get_duration(const animation& a) noexcept
	{
		return a.duration;
	}
}

namespace hades::resources
{
	enum class anim_map {
		bad,
		duration,
		pos,
		size,
		scale,
		offset
	};

	static animation_frame parse_frame(const string& name, const data::parser_node& f)
	{
		using namespace std::string_view_literals;
		const auto frame_info = f.get_children();
		auto frame = animation_frame{};
		auto found_pos = false;
		auto found_size = false;
		for (const auto& elm : frame_info)
		{
			const auto& key = elm->to_string();
			const auto& child = elm->get_child();
			if (!child)
			{
				LOGWARNING("animation: " + name + ", frame list missing data");
				continue;
			}

			const auto key_enum = [](const auto& key) {
				if (key == "pos"sv)
					return anim_map::pos;
				else if (key == "size"sv)
					return anim_map::size;
				else if (key == "scale"sv)
					return anim_map::scale;
				else if (key == "offset"sv)
					return anim_map::offset;
				else if (key == "duration"sv)
					return anim_map::duration;
				else
					return anim_map::bad;
			}(key);

			if (key_enum == anim_map::duration)
				frame.duration = child->to_scalar<float>();
			else if (key_enum == anim_map::bad)
			{
				LOGWARNING("animation: " + name + ", frame list bad data");
				continue;
			}
			else
			{
				const auto& seq = child->to_sequence<float>();
				if (size(seq) < 2)
				{
					LOGWARNING("animation: " + name + ", frame list bad data");
					continue;
				}

				switch (key_enum)
				{
				case anim_map::pos:
				{
					frame.x = seq[0];
					frame.y = seq[1];
					found_pos = true;
				}break;
				case anim_map::size:
				{
					frame.w = seq[0];
					frame.h = seq[1];
					found_size = true;
				}break;
				case anim_map::scale:
				{
					frame.scale_w = seq[0];
					frame.scale_h = seq[1];
				}break;
				case anim_map::offset:
				{
					frame.off_x = seq[0];
					frame.off_y = seq[1];
				}break;
				default:
					LOGWARNING("animation: " + name + ", frame list bad data");
				}// !switch
			}
		} // !for

		if (!(found_pos && found_size))
			LOGWARNING("animation: " + name + ", frame list bad data");

		return frame;
	}

	static animation_frame parse_frame_seq(const string& name, const data::parser_node& f, unique_id mod)
	{
		const auto frame_info = f.to_sequence<float>();
		const auto frame_size = size(frame_info);

		if (frame_size < 4)
		{
			const auto message = "Animation frame requires at least x, y, w and h components, in mod: " +
				to_string(mod) + ", for animation: " + name;
			LOGWARNING(message);
			return animation_frame{};
		}
		else if (frame_size > 9)
		{
			const auto message = "animation frame contains more that the expected data, ignoring excess, frame[x, y, w, h, scale_x, scale_y, offset_x, offset_y, rel_duration], in mod: " +
				to_string(mod) + ", for animation: " + name;
			LOGWARNING(message);
		}

		auto frame = animation_frame{
			frame_info[0],
			frame_info[1],
			frame_info[2],
			frame_info[3]
		};

		switch (std::min(frame_size, std::size_t{ 9 }))
		{
		case 9:
			frame.duration = frame_info[8];
			[[fallthrough]];
		case 8:
			frame.off_y = frame_info[7];
			[[fallthrough]];
		case 7:
			frame.off_x = frame_info[6];
			[[fallthrough]];
		case 6:
			frame.scale_h = frame_info[5];
			[[fallthrough]];
		case 5:
			frame.scale_w = frame_info[4];
		}

		return frame;
	}

	static void parse_animation(unique_id mod, const data::parser_node &n, data::data_manager &d)
	{
		//animations:
		//	example-animation:
		//		duration: 1.0s
		//		texture: example-texture
		//		width: +int
		//      height: +int
		//		loop: bool
		//		frames:
		//			- [x, y, w, h, scalex, scaley, offx, offy, d]
		//			the third parameter onward is optional
		// alt		- 
		//			    pos: [x, y]
		//				size: [w, h]
		//				scale: [x, y] //float, default [1.f, 1.f]
		//				offset: [x, y] //float, default [0, 0]
		//				duration: default 1.f
		//			
		

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
			namespace tex_funcs = texture_functions;
			const auto tex_id = anim->tex ? tex_funcs::get_id(anim->tex) : unique_id::zero;
			const auto new_tex_id = get_unique(*a, "texture"sv, tex_id);

			if (new_tex_id != unique_id::zero)
				anim->tex = tex_funcs::find_create_texture(d, new_tex_id, mod);

			const auto frames_node = a->get_child("frames"sv);

			if (!frames_node)
				continue;

			const auto frames = frames_node->get_children();

			auto total_duration = 0.f;
			auto frame_list = std::vector<animation_frame>{};
			frame_list.reserve(frames.size());

			for (const auto& f : frames)
			{
				auto frame = animation_frame{};
				if (f->is_map())
					frame = parse_frame(name, *f);
				else
					frame = parse_frame_seq(name, *f, mod);

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
		if (!texture_functions::get_is_loaded(a.tex))
		{
			const auto id = texture_functions::get_id(a.tex);
			//data->get will lazy load texture
			texture_functions::get_resource(d, id);
		}
	}
}

namespace hades::animation
{
	static const resources::animation_frame& get_frame(const resources::animation &animation, float progress)
	{
		//NOTE: based on the FrameAnimation algorithm from Thor C++
		//https://github.com/Bromeon/Thor/blob/master/include/Thor/Animations/FrameAnimation.hpp

		if (progress > 1.f || progress < 0.f)
		{
			LOGWARNING("Animation progress parameter must be in the range (0, 1) was: " + to_string(progress) +
				"; while setting animation: " + to_string(animation.id));
		}

		//force lazy load if the texture hasn't been loaded yet.
		// TODO: is this ever reached
		assert(resources::texture_functions::get_is_loaded(animation.tex));
		if (!resources::texture_functions::get_is_loaded(animation.tex))
		{
			const auto id = resources::texture_functions::get_id(animation.tex);
			//data->get will lazy load texture
			resources::texture_functions::get_resource(id);
		}
		
		//calculate the progress to find the correct rect for this time
		for (const auto &frame : animation.value)
		{
			progress -= frame.duration;

			// Must be <= and not <, to handle case (progress == frame.duration == 1) correctly
			if (progress <= 0.f)
				return  frame;
		}

		LOGWARNING("Unable to find correct frame for animation " + to_string(animation.id) +
			"animation progress was: " + to_string(progress));
		return animation.value.back();
	}

	const resources::animation_frame& get_frame(const resources::animation &animation, time_point t)
	{
		const auto ratio = normalise_time(t, animation.duration);
		return get_frame(animation, ratio);
	}

	void apply(const resources::animation& animation, float progress, sf::Sprite& target)
	{
		const auto& f = get_frame(animation, progress);
		const auto tex_x = f.w < 0 ? f.x + abs(f.w) : f.x;
		const auto tex_y = f.y < 0? f.y + abs(f.h) : f.y;
		const auto tex_w = f.w;
		const auto tex_h = f.h;

		target.setTexture(resources::texture_functions::get_sf_texture(animation.tex));
		target.setTextureRect({ 
			static_cast<int>(tex_x),
			static_cast<int>(tex_y),
			static_cast<int>(tex_w),
			static_cast<int>(tex_h)
		});
	
		target.setOrigin({ f.off_x, f.off_y });
		target.setScale({ f.scale_w, f.scale_h });
		return;
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
		const auto col = to_sf_color(c);

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

	poly_quad make_quad_animation(const vector_float p, const resources::animation_frame &f) noexcept
	{
		return make_quad_animation(p, {f.w, f.h}, f);
	}

	poly_quad make_quad_animation(const vector_float pos, const vector_float size, const resources::animation_frame& f) noexcept
	{
		const auto offset = vector_float{ f.off_x * f.scale_w, f.off_y * f.scale_h };
		const auto scaled_size = vector_float{ size.x * f.scale_w, size.y * f.scale_h };

		return make_quad_animation({ pos - offset, scaled_size }, {
			f.w < 0 ? f.x + abs(f.w) : f.x, 
			f.h < 0 ? f.y + abs(f.h) : f.y,
			f.w, f.h 
		});
	}

	poly_quad make_quad_animation(const rect_float quad, const rect_float texture_quad) noexcept
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