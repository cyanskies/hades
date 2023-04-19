#include "hades/animation.hpp"

#include <cassert>

#include "hades/data.hpp"
#include "hades/logging.hpp"
#include "hades/parser.hpp"
#include "hades/sf_color.hpp"
#include "hades/texture.hpp"
#include "hades/timers.hpp"
#include "hades/writer.hpp"

using namespace std::string_view_literals;
constexpr auto anim_str = "animations"sv;
constexpr auto anim_group_str = "animation-groups"sv;

constexpr auto dur_str = "duration"sv;
constexpr auto tex_str = "texture"sv;
constexpr auto frames_str = "frames"sv;

constexpr auto pos_str = "pos"sv;
constexpr auto size_str = "size"sv;
constexpr auto scale_str = "scale"sv;
constexpr auto offset_str = "offset"sv;

namespace hades::resources
{
	struct animation;
	static void load_animation(animation&, data::data_manager&);
	//TODO: add field for fragment shaders
	struct animation : public resource_type<std::vector<animation_frame>>
	{
		void load(data::data_manager& d) final override
		{
			load_animation(*this, d);
			return;
		}

		void serialise(const data::data_manager&, data::writer&) const override;

		resource_link<texture> tex;
		time_duration duration = time_duration::zero();
		//const shader *anim_shader = nullptr;
	};

	//using anim_group_base = ;
	struct animation_group;
	static void load_anim_group(animation_group&, data::data_manager&);

	struct animation_group : public resource_type<std::unordered_map<unique_id, resource_link<animation>>>
	{
		void load(data::data_manager& d) final override
		{
			load_anim_group(*this, d);
			return;
		}
	};

	static void normalise_durations(std::vector<animation_frame>& frame_list)
	{
		const auto total_duration = std::transform_reduce(begin(frame_list),
			end(frame_list), 0.f, std::plus<>(), std::mem_fn(&animation_frame::duration));
		assert(total_duration != 0.f);
		for (auto& f : frame_list)
			f.normalised_duration = f.duration / total_duration;
	}

	void animation::serialise(const data::data_manager& d, data::writer& w) const
	{
		const auto def_anim = animation{};
		const auto def = animation_frame{};

		w.start_map(d.get_as_string(id));
		if (duration != def_anim.duration)
			w.write(dur_str, duration);
		if (tex)						
			w.write(tex_str, d.get_as_string(tex.id()));

		if (!empty(value))
		{
			w.start_sequence(frames_str);
			{
				for (const auto& f : value)
				{
					w.start_map();
					if (f.x != def.x || f.y != def.y)
					{
						w.write(pos_str);
						w.start_sequence();
						w.write(f.x);
						w.write(f.y);
						w.end_sequence();
					}

					if (f.w != def.w || f.h != def.h)
					{
						w.write(size_str);
						w.start_sequence();
						w.write(f.w);
						w.write(f.h);
						w.end_sequence();
					}

					if (f.scale_w != def.scale_w || f.scale_h != def.scale_h)
					{
						w.write(scale_str);
						w.start_sequence();
						w.write(f.scale_w);
						w.write(f.scale_h);
						w.end_sequence();
					}

					if (f.off_x != def.off_x || f.off_y != def.off_y)
					{
						w.write(offset_str);
						w.start_sequence();
						w.write(f.off_x);
						w.write(f.off_y);
						w.end_sequence();
					}

					if (f.duration != def.duration)
						w.write(dur_str, f.duration);
					
					w.end_map();
				}
			}
			w.end_sequence();
		}
		w.end_map();

		return;
	}
}

namespace hades::resources::animation_functions
{
	const animation* get_resource(unique_id id)
	{
		return data::get<animation>(id);
	}

	animation* get_resource(data::data_manager& d, unique_id id, std::optional<unique_id> mod)
	{
		return d.get<animation>(id, mod);
	}

	resource_link<animation> make_resource_link(data::data_manager& d, unique_id id, unique_id from)
	{
		return d.make_resource_link<animation>(id, from, [](data::data_manager& d, unique_id target)->const animation* {
			return d.get<animation>(target, data::no_load);
			});
	}

	std::vector<resource_link<animation>> make_resource_link(data::data_manager& d, const std::vector<unique_id>& ids, unique_id from)
	{
		return d.make_resource_link<animation>(ids, from, [](data::data_manager& d, unique_id target)->const animation* {
			return d.get<animation>(target, data::no_load);
			});
	}

	try_get_return try_get(unique_id id) noexcept
	{
		return data::try_get<animation>(id);
	}

	const animation* find_or_create(data::data_manager& d, unique_id id, std::optional<unique_id> mod)
	{
		return d.find_or_create<animation>(id, mod, anim_str);
	}

	std::vector<const animation*> find_or_create(data::data_manager& d, const std::vector<unique_id>& ids, std::optional<unique_id> mod)
	{
		return d.find_or_create<const animation>(ids, mod, anim_str);
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
		if(a.tex.is_linked())
			return a.tex.get();
		return nullptr;
	}

	time_duration get_duration(const animation& a) noexcept
	{
		return a.duration;
	}

	const std::vector<animation_frame>& get_animation_frames(const animation& a) noexcept
	{
		return a.value;
	}

	void set_animation_frames(animation& a, std::vector<animation_frame> frames)
	{
		a.value = std::move(frames);
		normalise_durations(a.value);
		return;
	}

	void set_duration(animation& a, time_duration dur) noexcept
	{
		a.duration = dur;
		return;
	}

	vector_float get_minimum_offset(const animation& a) noexcept
	{
		auto ret = vector_float{};
		for (const auto& f : a.value)
		{
			ret.x = std::min(ret.x, f.off_x);
			ret.y = std::min(ret.y, f.off_y);
		}
		return ret;
	}

	rect_float get_bounding_area(const animation& a, vector_float size) noexcept
	{
		auto ret = rect_float{};
		for(const auto& f : a.value)
		{
			ret.x = std::min(ret.x, f.off_x);
			ret.y = std::min(ret.y, f.off_y);
			ret.width = std::max(ret.width, f.off_x + size.x * std::abs(f.scale_w));
			ret.height = std::max(ret.height, f.off_y + size.y * std::abs(f.scale_h));
		}
		return ret;
	}
}

namespace hades::resources::animation_group_functions
{
	const animation_group* get_resource(const unique_id id, const std::optional<unique_id> mod)
	{
		return data::get<animation_group>(id, mod);
	}

	animation_group* get_resource(data::data_manager& d, const unique_id i, const std::optional<unique_id> mod)
	{
		return d.get<animation_group>(i, mod);
	}

	resource_link<animation_group> make_resource_link(data::data_manager& d, const unique_id id, const unique_id from)
	{
		return d.make_resource_link<animation_group>(id, from, [](data::data_manager& d, const unique_id target)-> const animation_group* {
			return d.get<animation_group>(target, data::no_load);
			});
	}

	try_get_return try_get(unique_id i) noexcept
	{
		return data::try_get<animation_group>(i);
	}

	const animation_group* find_or_create(data::data_manager& d, unique_id i, std::optional<unique_id> mod)
	{
		return d.find_or_create<animation_group>(i, mod, anim_group_str);
	}

	bool is_loaded(const animation_group& a) noexcept
	{
		return a.loaded;
	}

	unique_id get_id(const animation_group& a) noexcept
	{
		return a.id;
	}

	const animation* get_animation(const animation_group& a, unique_id i)
	{
		const auto iter = a.value.find(i);
		if (iter == end(a.value))
			return nullptr;

		return iter->second.get();
	}

	std::vector<std::pair<unique_id, unique_id>> get_all_animations(const animation_group& g)
	{
		auto out = std::vector<std::pair<unique_id, unique_id>>{};
		for (const auto& [name, elm] : g.value)
			out.emplace_back(name, elm.id());
		return out;
	}

	void replace_animations(animation_group& a, std::unordered_map<unique_id, resource_link<animation>> data)
	{
		a.value = std::move(data);
		return;
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
		auto found_size = false;
		for (const auto& elm : frame_info)
		{
			const auto& key = elm->to_string();
			const auto& children = elm->get_children();
			if (empty(children))
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
			{
				if (size(children) != 1)
				{
					LOGWARNING("animation: " + name + ", 'duration' is corrupt");
					continue;
				}

				frame.duration = children[0]->to_scalar<float>();
			}
			else if (key_enum == anim_map::bad)
			{
				LOGWARNING("animation: " + name + ", frame list bad data");
				continue;
			}
			else
			{
				const auto& seq = elm->to_sequence<float>();
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

		if (!found_size)
			LOGWARNING("animation: " + name + ", missing frame size");

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
        //constexpr auto resource_type = "animation"sv;

		const auto animations = n.get_children();

		for (const auto &a : animations)
		{
			const auto name = a->to_string();
			const auto id = d.get_uid(name);

			auto anim = d.find_or_create<animation>(id, mod, anim_str);

			using namespace data::parse_tools;
			anim->duration = get_scalar(*a, "duration"sv, anim->duration, duration_from_string);
			namespace tex_funcs = texture_functions;
			const auto tex_id = anim->tex ? anim->tex.id() : unique_id::zero;
			const auto new_tex_id = get_unique(*a, "texture"sv, tex_id);

			if (new_tex_id != unique_id::zero)
				anim->tex = texture_functions::make_resource_link(d, new_tex_id, id);

			const auto frames_node = a->get_child("frames"sv);

			if (!frames_node)
				continue;

			const auto frames = frames_node->get_children();

			auto frame_list = std::vector<animation_frame>{};
			frame_list.reserve(frames.size());

			for (const auto& f : frames)
			{
				auto frame = animation_frame{};
				if (!f->is_sequence())
					frame = parse_frame(name, *f);
				else
					frame = parse_frame_seq(name, *f, mod);

				frame_list.emplace_back(std::move(frame));
			}//for frames

			//normalise the frame durations
			normalise_durations(frame_list); // NOTE: this is broke

			std::swap(anim->value, frame_list);
		}//for animations
	}//parse animations

	static void parse_animation_group(unique_id mod, const data::parser_node& n, data::data_manager& d)
	{
		//animation-groups:
		//	infantry_anims:
		//		look-left: left-anim
		//		look-right: right-anim
		//			

		using namespace std::string_literals;
		using namespace std::string_view_literals;
		
		const auto animations = n.get_children();

		for (const auto& a : animations)
		{
			const auto map = a->to_map<unique_id>(data::make_uid);
			const auto group_name = a->to_string();
			const auto group_id = d.get_uid(group_name);
			auto group = d.find_or_create<animation_group>(group_id, mod, anim_group_str);
			for (const auto& [name, val] : map)
			{
				const auto id = d.get_uid(name);
				group->value.insert_or_assign(id, animation_functions::make_resource_link(d, val, group_id));
			}
		}

		return;
	}

	static void load_animation(animation&a, data::data_manager &d)
	{
		using namespace std::string_literals;
		if (!a.tex)
			log_warning("Failed to load animation: " + d.get_as_string(a.id) + ", missing texture"s);
		else if (!texture_functions::get_is_loaded(a.tex.get()))
		{
			//data->get will lazy load texture
			texture_functions::get_resource(d, a.tex.id());
		}
	}

	static void load_anim_group(animation_group& r, data::data_manager& d)
	{
		for (const auto& anim : r.value)
			animation_functions::get_resource(d, anim.second->id);
		return;
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

		// Warn if texture isn't loaded.
		// this can only happen if we got the animation without allowing 
		// autoload. Usually in an editor. Get the texture and manually load it
		// before calling this type of dunction.
		if (!resources::texture_functions::get_is_loaded(animation.tex.get()))
		{
			log_warning("texture not loaded for animation: " + to_string(animation.id));
		}
		
		//calculate the progress to find the correct rect for this time
		for (const auto &frame : animation.value)
		{
			progress -= frame.normalised_duration;

			// Must be <= and not <, to handle case (progress == frame.duration == 1) correctly
			if (progress <= 0.f)
				return  frame;
		}

		assert(!empty(animation.value));
		LOGWARNING("Unable to find correct frame for animation " + to_string(animation.id) +
			"animation progress was: " + to_string(progress));
		return animation.value.back();
	}

	const resources::animation_frame& get_frame(const resources::animation &animation, time_point t)
	{
		const auto ratio = normalise_time(t, animation.duration);
		return get_frame(animation, ratio);
	}

	const resources::animation_frame& get_frame(const resources::animation& animation, int32 frame)
	{
		const auto& frames = resources::animation_functions::get_animation_frames(animation);
		const auto size = std::size(frames);
		// support wrapping in either direction [-int, int]
		return frames[(frame % size + size) % size];
	}

	void apply(const resources::animation& animation, float progress, sf::Sprite& target)
	{
		const auto& f = get_frame(animation, progress);
        const auto tex_x = f.w < 0 ? f.x + std::abs(f.w) : f.x;
        const auto tex_y = f.y < 0? f.y + std::abs(f.h) : f.y;
		const auto tex_w = f.w;
		const auto tex_h = f.h;

		target.setTexture(resources::texture_functions::get_sf_texture(animation.tex.get()));
		target.setTextureRect({{
				static_cast<int>(tex_x),
				static_cast<int>(tex_y)
			}, {
				static_cast<int>(tex_w),
				static_cast<int>(tex_h)
			}});
	
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
            f.w < 0 ? f.x + std::abs(f.w) : f.x,
            f.h < 0 ? f.y + std::abs(f.h) : f.y,
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

		d.register_resource_type(anim_str, resources::parse_animation);
		d.register_resource_type(anim_group_str, resources::parse_animation_group);
	}
}
