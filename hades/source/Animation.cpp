#include "Hades/Animation.hpp"

#include <cassert>

#include "Hades/Data.hpp"
#include "Hades/Logging.hpp"

namespace hades
{
	void animation_error(float progress, const resources::animation* const animation)
	{
		LOGWARNING("Animation progress parameter must be in the range (0, 1) was: " + std::to_string(progress) +
			"; while setting animation: " + data::get_as_string(animation->id));
	}

	namespace animation
	{
		animation_frame GetFrame(const resources::animation* animation, float progress)
		{
			if (progress > 1.f || progress < 0.f)
				animation_error(progress, animation);

			//force lazy load if the texture hasn't been loaded yet.
			if (!animation->tex->loaded)
				hades::data::get<hades::resources::texture>(animation->tex->id);

			auto prog = std::clamp(progress, 0.f, 1.f);

			//calculate the progress to find the correct rect for this time
			for (auto &frame : animation->value)
			{
				prog -= frame.duration;

				// Must be <= and not <, to handle case (progress == frame.duration == 1) correctly
				if (prog <= 0.f)
					return std::make_tuple(static_cast<float>(frame.x), static_cast<float>(frame.y));
			}

			LOGWARNING("Unable to find correct frame for animation " + data::get_as_string(animation->id) +
				"animation progress was: " + std::to_string(progress));
			return std::make_tuple(0.f, 0.f);
		}

		animation_frame GetFrame(const resources::animation* animation, sf::Time t)
		{
			assert(animation);
			return GetFrame(animation, ConvertToRatio(t, animation->duration));
		}

		//NOTE: based on the FrameAnimation algorithm from Thor C++
		//https://github.com/Bromeon/Thor/blob/master/include/Thor/Animations/FrameAnimation.hpp
		void Apply(const resources::animation* animation, float progress, sf::Sprite &target)
		{
			assert(animation);

			const auto [x, y] = GetFrame(animation, progress);
			target.setTexture(animation->tex->value);
			target.setTextureRect({ static_cast<int>(x), static_cast<int>(y) , animation->width, animation->height });
		}
	}
}
