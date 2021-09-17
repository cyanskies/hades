#ifndef HADES_ANIMATION_HPP
#define HADES_ANIMATION_HPP

#include <array>

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/colour.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/resource_base.hpp"
#include "hades/texture.hpp"
#include "hades/timers.hpp"

//forwards
namespace hades::data
{
	class data_manager;
}

namespace hades::resources
{
	struct shader;

	struct animation_frame
	{
		using texture_diff_t = std::make_signed_t<texture_size_t>;
		//the rectangle for this frame and the duration relative to the rest of the frames in this animation
		texture_size_t x = 0, y = 0;
		bool flip_x = false, flip_y = false; // TODO: turn these into stretch values [-x, +x], use negative values to flip
		//texture_diff_t scale_x = 1, scale_y = 1; // perhaps float is more appropriate
		// TODO: add origin offset
		//texture_diff_t off_x = 0, off_y = 0;
		float duration = 1.f;
	};

	//TODO: add field for fragment shaders
	struct animation : public resource_type<std::vector<animation_frame>>
	{
		animation() noexcept;

		const texture* tex = nullptr;
		time_duration duration = time_duration::zero();
		types::int32 width = 0, height = 0;
		//const shader *anim_shader = nullptr;
	};
}

namespace hades::animation
{
	struct animation_frame_data {
		int32 x, y, w, h;
	};

	constexpr bool operator==(animation_frame_data l, animation_frame_data r) noexcept
	{
		return std::tie(l.x, l.y, l.w, l.h) == std::tie(r.x, r.y, r.w, r.h);
	}

	constexpr bool operator!=(animation_frame_data l, animation_frame_data r) noexcept
	{
		return !(l == r);
	}

	//returns the first frame of the animation, or the animation for the requested time, with wrapping
	animation_frame_data get_frame(const resources::animation &animation, time_point time_played);

	//applys an animation to a sprite, progress is a float in the range (0, 1), indicating how far into the animation the sprite should be set to.
	void apply(const resources::animation &animation, float progress, sf::Sprite &target);
	void apply(const resources::animation &animation, time_point progress, sf::Sprite &target);
}

namespace hades
{
	constexpr static auto quad_vert_count = std::size_t{ 6 };
	using poly_quad = std::array<sf::Vertex, quad_vert_count>;
	poly_quad make_quad_colour(rect_float quad, colour) noexcept;
	poly_quad make_quad_animation(vector_float pos, const resources::animation&, const animation::animation_frame_data&) noexcept;
	poly_quad make_quad_animation(vector_float pos, vector_float size, const animation::animation_frame_data&) noexcept;
	poly_quad make_quad_animation(rect_float quad, rect_float texture_quad) noexcept;

	//functions for updating vertex data
	using poly_raw = poly_quad::pointer;

	void register_animation_resource(data::data_manager&);
}

#endif //HADES_ANIMATION_HPP
