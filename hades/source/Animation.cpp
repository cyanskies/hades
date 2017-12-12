#include "Hades/Animation.hpp"

#include <cassert>

#include "Hades/Data.hpp"
#include "Hades/DataManager.hpp"
#include "Hades/Logging.hpp"

namespace hades
{
	void animation_error(float progress, const resources::animation* const animation)
	{
		LOGWARNING("Animation progress parameter must be in the range (0, 1) was: " + std::to_string(progress) +
			"; while setting animation: " + data::GetAsString(animation->id));
	}

	//NOTE: based on the FrameAnimation algorithm from Thor C++
	//https://github.com/Bromeon/Thor/blob/master/include/Thor/Animations/FrameAnimation.hpp
	void apply_animation(const resources::animation* const animation, float progress, sf::Sprite &target)
	{
		assert(animation);

		//test variants
		//this version clamped and warned if the progress was outside of the range
		if(progress > 1.f)
		{
			animation_error(progress, animation);
			progress = 1.f;
		}
		else if(progress < 0.f)
		{
			animation_error(progress, animation);
			progress = 0.f;
		}

		//set the texture
		if(!animation->value.empty())
			target.setTexture(animation->tex->value);
		else
		{
			target.setTexture(animation->tex->value, true);
			return;
		}

		auto rect = target.getTextureRect();
		//calculate the progress to find the correct rect for this time
		for (auto &frame : animation->value)
		{
			progress -= frame.duration;

			// Must be <= and not <, to handle case (progress == frame.duration == 1) correctly
			if (progress <= 0.f)
			{
				target.setTextureRect({ static_cast<types::int32>(frame.x),
					static_cast<types::int32>(frame.y), animation->width, animation->height });
				//if (frame.applyOrigin)
					//target.setOrigin(frame.origin);

				break;
			}
		}
	}
}
