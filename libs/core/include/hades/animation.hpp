#ifndef HADES_ANIMATION_HPP
#define HADES_ANIMATION_HPP

#include <array>

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/colour.hpp"
#include "hades/math.hpp"
#include "hades/texture.hpp"
#include "hades/timers.hpp"

namespace hades::resources
{
	struct shader;

	struct animation_frame
	{
		//the rectangle for this frame and the duration relative to the rest of the frames in this animation
		texture::size_type x = 0, y = 0;
		float duration = 1.f;
	};

	//TODO: add field for fragment shaders
	struct animation : public resource_type<std::vector<animation_frame>>
	{
		animation();

		const texture* tex = nullptr;
		time_duration duration;
		types::int32 width = 0, height = 0;
		const shader *anim_shader = nullptr;
	};
}

namespace hades::animation
{
	using animation_frame = std::tuple<float, float>; /*x and y*/

	//returns the first frame of the animation, or the animation for the requested time, with wrapping
	animation_frame get_frame(const resources::animation &animation, time_point time_played);

	//applys an animation to a sprite, progress is a float in the range (0, 1), indicating how far into the animation the sprite should be set to.
	void apply(const resources::animation &animation, float progress, sf::Sprite &target);
	void apply(const resources::animation &animation, time_point progress, sf::Sprite &target);
}

namespace hades
{
	using poly_quad = std::array<sf::Vertex, 6u>;
	poly_quad make_quad_colour(rect_float quad, colour);
	poly_quad make_quad_animation(rect_float quad, rect_float texture_quad);

	void register_animation_resource(data::data_manager&);
}

#endif //HADES_ANIMATION_HPP
