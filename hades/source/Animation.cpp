#include "Hades/Animation.hpp"

#include <cassert>

#include "Hades/DataManager.hpp"
#include "Hades/Logging.hpp"

namespace hades
{
	void animation_error(float progress, const resources::animation* const animation)
	{
		LOGWARNING("Animation progress parameter must be in the range (0, 1) was: " + std::to_string(progress) + 
			"; while setting animation: " + data_manager->as_string(animation->id));
	}

	//NOTE: based on the FrameAnimation algorithm from Thor C++
	//https://github.com/Bromeon/Thor/blob/master/include/Thor/Animations/FrameAnimation.hpp
	void apply_animation(const resources::animation* const animation, float progress, sf::Sprite &target)
	{
		assert(animation);

		//test variants
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
		auto texture = data_manager->getTexture(animation->texture);
		target.setTexture(texture->value);

		auto rect = target.getTextureRect();
		//calculate the progress to find the correct rect for this time
		for (auto &frame : animation->value)
		{
			progress -= frame.duration;

			// Must be <= and not <, to handle case (progress == frame.duration == 1) correctly
			if (progress <= 0.f)
			{
				target.setTextureRect({ frame.x, frame.y, frame.width, frame.height });
				//if (frame.applyOrigin)
					//target.setOrigin(frame.origin);

				break;
			}
		}
	}
}