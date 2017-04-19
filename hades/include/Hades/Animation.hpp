#ifndef HADES_ANIMATION_HPP
#define HADES_ANIMATION_HPP

#include "SFML/Graphics/Sprite.hpp"

#include "simple_resources.hpp"

namespace hades
{
	void apply_animation(resources::animation *animation, float progress, sf::Sprite &target);
}

#endif //HADES_ANIMATION_HPP
