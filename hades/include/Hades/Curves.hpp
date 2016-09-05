#ifndef HADES_CURVES_HPP
#define HADES_CURVES_HPP

namespace hades {

	class Curve {
		enum Type {LINEAR, //data between keyframes is exactly the difference between them
			STEP, // data between keyframes is identical to the previous keyframe
			PULSE // data between keyframes is null
		};
	};
}

#endif //HADES_CURVES_HPP
