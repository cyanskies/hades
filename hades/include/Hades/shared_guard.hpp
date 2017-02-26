#ifndef HADES_SHAREDGUARD_HPP
#define HADES_SHAREDGUARD_HPP

#include <shared_mutex>

#include "value_guard.hpp"

// a thread safe wrapper for arbitary value types.
// support concurrent reads for values that do not change often
namespace hades {
	template<typename value, typename mutex = std::shared_mutex>
	class shared_guard : public value_guard<value, mutex>
	{
	public:
		shared_guard() = default;
		constexpr shared_guard(value desired) : value_guard(desired) {}
		shared_guard(const shared_guard&) = delete;
		void operator=(const shared_guard&) = delete;

		operator value()
		{
			std::shared_lock<mutex>(_mutex);
			return _value;
		}
	};
}

#endif // !HADES_SHAREDGUARD_HPP
