#ifndef HADES_ANIMATION_HPP
#define HADES_ANIMATION_HPP

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/System/Time.hpp"

#include "simple_resources.hpp"

namespace hades
{
	//applys an animation to a sprite or vertex array view, 't' is the time since the animation started, looping if it is longer than duration.
	template<typename Drawable>
	void apply_animation(const resources::animation* const animation, sf::Time t, Drawable &target)
	{
		assert(animation);

		auto d = sf::seconds(animation->duration);
		if (t > d)
		{
			while (t > d)
				t -= d;
		}

		auto progress = t.asSeconds() / d.asSeconds();

		apply_animation(animation, progress, target);
	}

	//applys an aniamtion to a sprite, progress is a float in the range (0, 1), indicating how far into the animation the sprite should be set to.
	void apply_animation(const resources::animation* const animation, float progress, sf::Sprite &target);
	//TODO: version of apply_animation that works on vertex[4]


}

#endif //HADES_ANIMATION_HPP
