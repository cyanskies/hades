#ifndef HADES_CAPABILITIES_HPP
#define HADES_CAPABILITIES_HPP

#include "hades/exceptions.hpp"

namespace hades
{
	// throw if the system is lacking any needed capabilities
	class capability_missing_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	// throws if a capability is missing
	void test_capabilities();
}

#endif //!HADES_CAPABILITIES_HPP
