#ifndef HADES_ANIMATION_HPP
#define HADES_ANIMATION_HPP

#include <array>

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/colour.hpp"
#include "hades/data.hpp" // TODO: create data_fwd header, we only include this for try_get_return and get_error
#include "hades/rectangle_math.hpp"
#include "hades/uniqueid.hpp"
#include "hades/time.hpp"

//forwards
namespace hades::data
{
	class data_manager;
}

namespace hades::resources
{
	//struct shader;
	struct animation_frame
	{
		float x, y; // tex coords(px)
		float w, h; // tex coords(px)
		float scale_w = 1.f, scale_h = 1.f; // draw scaling (ratio), negative to flip
		float off_x = 0.f, off_y = 0.f; // draw origin offset
		float duration = 1.f; // relative frame duration
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
	struct texture;

	namespace animation_functions
	{
		using try_get_return = data::data_manager::try_get_return<const animation>;
		const animation* get_resource(unique_id);
		const animation* get_resource(data::data_manager&, unique_id);
		try_get_return try_get(unique_id) noexcept;
		const animation* find_or_create(data::data_manager&, unique_id, unique_id mod);
		std::vector<const animation*> find_or_create(data::data_manager&, const std::vector<unique_id>&, unique_id mod);
		bool is_loaded(const animation&) noexcept;
		unique_id get_id(const animation&) noexcept;
		std::vector<unique_id> get_id(const std::vector<const animation*>&) noexcept;
		const texture* get_texture(const animation&) noexcept;
		time_duration get_duration(const animation&) noexcept;
		vector_int get_size(const animation&) noexcept;
		// const shader* get_shader(const animation&) noexcept;
	}
}

namespace hades::animation
{
	//returns the first frame of the animation, or the animation for the requested time, with wrapping
	const resources::animation_frame& get_frame(const resources::animation &animation, time_point time_played);

	//applys an animation to a sprite, progress is a float in the range (0, 1), indicating how far into the animation the sprite should be set to.
	void apply(const resources::animation &animation, float progress, sf::Sprite &target);
	void apply(const resources::animation &animation, time_point progress, sf::Sprite &target);
}

namespace hades
{
	constexpr static auto quad_vert_count = std::size_t{ 6 };
	using poly_quad = std::array<sf::Vertex, quad_vert_count>;
	poly_quad make_quad_colour(rect_float quad, colour) noexcept;
	poly_quad make_quad_animation(vector_float pos, const resources::animation_frame&) noexcept;
	poly_quad make_quad_animation(vector_float pos, vector_float size, const resources::animation_frame&) noexcept;
	poly_quad make_quad_animation(rect_float quad, rect_float texture_quad) noexcept;

	//functions for updating vertex data
	//using poly_raw = poly_quad::pointer;

	void register_animation_resource(data::data_manager&);
}

#endif //HADES_ANIMATION_HPP
