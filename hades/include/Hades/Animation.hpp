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
		using animation_frame = std::tuple < const hades::resources::texture*,
			hades::types::int32, hades::types::int32, /*x and y*/
			hades::types::int32, hades::types::int32 >; /*width and height*/

		//returns the first frame of the animation, or the animation for the requested time, with wrapping
		animation_frame GetFrame(const resources::animation* animation, sf::Time t = sf::Time::Zero);

		//applys an aniamtion to a sprite, progress is a float in the range (0, 1), indicating how far into the animation the sprite should be set to.
		void Apply(const resources::animation* animation, float progress, sf::Sprite &target);
		//TODO: version of apply_animation that works on vertex[4]

		//applys an animation to a sprite or vertex array view, 't' is the time since the animation started, looping if it is longer than duration.
		template<typename Drawable>
		void Apply(const resources::animation* animation, sf::Time t, Drawable &target)
		{
			assert(animation);
			auto progress = ConvertToRatio(t, animation->duration);
			Apply(animation, progress, target);
		}
	}
}

#endif //HADES_ANIMATION_HPP
