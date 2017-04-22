#ifndef HADES_ANIMATION_HPP
#define HADES_ANIMATION_HPP

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/System/Time.hpp"

#include "simple_resources.hpp"

namespace hades
{
	//applys an animation to a sprite, 't' is the time since the animation started, looping if it is longer than duration.
	void apply_animation(const resources::animation* const animation, sf::Time t, sf::Sprite &target);
	//applys an aniamtion to a sprite, progress is a float in the range (0, 1), indicating how far into the animation the sprite should be set to.
	void apply_animation(const resources::animation* const animation, float progress, sf::Sprite &target);
}

#endif //HADES_ANIMATION_HPP
