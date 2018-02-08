#ifndef HADES_ANIMATION_HPP
#define HADES_ANIMATION_HPP

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/System/Time.hpp"

#include "simple_resources.hpp"

namespace
{
	float ConvertToRatio(sf::Time t, float duration)
	{
		//reduce t until it is less than one duration
		auto d = sf::seconds(duration);
		if (t > d)
		{
			while (t > d)
				t -= d;
		}

		return t.asSeconds() / d.asSeconds();
	}
}

namespace hades
{
	namespace animation
	{
		using animation_frame = std::tuple<float, float>; /*x and y*/

		//returns the first frame of the animation, or the animation for the requested time, with wrapping
		animation_frame GetFrame(const resources::animation* animation, sf::Time t = sf::Time::Zero);

		//applys an aniamtion to a sprite, progress is a float in the range (0, 1), indicating how far into the animation the sprite should be set to.
		void Apply(const resources::animation* animation, float progress, sf::Sprite &target);
	}
}

#endif //HADES_ANIMATION_HPP
