#ifndef HADES_ANIMATION_HPP
#define HADES_ANIMATION_HPP

#include <array>

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/colour.hpp"
// TODO: create data_fwd header, we only include this for try_get_return and get_error
// NOTE: now we need it for resource link too
//		could be used in a few other places too
#include "hades/data.hpp" 
#include "hades/rectangle_math.hpp"
#include "hades/shader.hpp"
#include "hades/uniqueid.hpp"
#include "hades/time.hpp"

namespace hades::resources
{
	class animation_no_shader : public data::resource_error
	{
	public:
		using resource_error::resource_error;
	};

	//struct shader;
	struct animation_frame
	{
		float x{}, y{}; // tex coords(px)
		float w{}, h{}; // tex coords(px)
		float scale_w = 1.f, scale_h = 1.f; // draw scaling (ratio), negative to flip
		float off_x = {}, off_y = {}; // draw origin offset
		float duration = 1.f; // relative frame duration
		// NOTE: this is a generated value, don't edit it manually
		float normalised_duration = 0.f; //calculated normal duration
	};

	constexpr bool operator==(const animation_frame l, const animation_frame r) noexcept
	{
		return std::tie(l.x, l.y, l.w, l.h, l.scale_w, l.scale_h, l.off_x, l.off_y, l.duration)
			== std::tie(r.x, r.y, r.w, r.h, r.scale_w, r.scale_h, r.off_x, r.off_y, r.duration);
	}

	constexpr bool operator!=(const animation_frame l, const animation_frame r) noexcept
	{
		return !(l == r);
	}

	struct animation;
	struct animation_group;
	struct texture;

	namespace animation_functions
	{
		using try_get_return = data::data_manager::try_get_return<const animation>;
		const animation* get_resource(unique_id);
		animation* get_resource(data::data_manager&, unique_id, std::optional<unique_id> = {});
		resource_link<animation> make_resource_link(data::data_manager&, unique_id, unique_id from);
		std::vector<resource_link<animation>> make_resource_link(data::data_manager&, const std::vector<unique_id>&, unique_id from);
		try_get_return try_get(unique_id) noexcept;
		animation* find_or_create(data::data_manager&, unique_id, std::optional<unique_id> mod = {});
		std::vector<const animation*> find_or_create(data::data_manager&, const std::vector<unique_id>&, std::optional<unique_id> mod = {});
		bool is_loaded(const animation&) noexcept;
		unique_id get_id(const animation&) noexcept;
		resource_base* get_resource_base(animation&) noexcept;
		std::vector<unique_id> get_id(const std::vector<const animation*>&);
		const texture* get_texture(const animation&) noexcept;
		time_duration get_duration(const animation&) noexcept;
		const std::vector<animation_frame>& get_animation_frames(const animation&) noexcept;
		void set_animation_frames(animation&, std::vector<animation_frame>);
		void set_duration(animation&, time_duration) noexcept;
		vector2_float get_minimum_offset(const animation&) noexcept;
		// calculates the total area the animation might cover
		// accounting for origin offsets and scale changes
		rect_float get_bounding_area(const animation&, vector2_float size) noexcept;
		bool has_shader(const animation&) noexcept;
		// returns unique_id::zero if no shader
		unique_id get_shader_id(const animation&) noexcept;
		// THROWS: animation_no_shader if the animation doesn't have a shader
		//	or hasn't been loaded properly; use has_shader to check
		shader_proxy get_shader_proxy(const animation&);
	}

	namespace animation_group_functions
	{
		using try_get_return = data::data_manager::try_get_return<const animation_group>;
		const animation_group* get_resource(unique_id, std::optional<unique_id> mod = {});
		animation_group* get_resource(data::data_manager& d, unique_id id, std::optional<unique_id> mod = {});
		resource_link<animation_group> make_resource_link(data::data_manager&, unique_id, unique_id from);
		try_get_return try_get(unique_id) noexcept;
		animation_group* find_or_create(data::data_manager&, unique_id, std::optional<unique_id> mod);
		bool is_loaded(const animation_group&) noexcept;
		unique_id get_id(const animation_group&) noexcept;
		resource_base* get_resource_base(animation_group&) noexcept;
		const animation* get_animation(const animation_group&, unique_id);
		std::vector<std::pair<unique_id, unique_id>> get_all_animations(const animation_group&);
		void replace_animations(animation_group&, std::unordered_map<unique_id, resource_link<animation>>);
	}
}

namespace hades::animation
{
	//returns the first frame of the animation, or the animation for the requested time, with wrapping
	const resources::animation_frame& get_frame(const resources::animation &animation, time_point time_played);
	const resources::animation_frame& get_frame(const resources::animation& animation, int32 frame);

	//applys an animation to a sprite, progress is a float in the range (0, 1), indicating how far into the animation the sprite should be set to.
	void apply(const resources::animation &animation, float progress, sf::Sprite &target);
	void apply(const resources::animation &animation, time_point progress, sf::Sprite &target);
}

namespace hades
{
	constexpr static auto quad_vert_count = std::size_t{ 6 };
	using poly_quad = std::array<sf::Vertex, quad_vert_count>;
	poly_quad make_quad_colour(rect_float quad, colour) noexcept;
	poly_quad make_quad_line(vector2_float start, vector2_float end, float thickness, colour) noexcept;
	poly_quad make_quad_animation(vector2_float pos, const resources::animation_frame&) noexcept;
	poly_quad make_quad_animation(vector2_float pos, vector2_float size, const resources::animation_frame&) noexcept;
	poly_quad make_quad_animation(rect_float quad, rect_float texture_quad, std::array<colour, 4> = {}) noexcept;

	void register_animation_resource(data::data_manager&);
}

#endif //HADES_ANIMATION_HPP
