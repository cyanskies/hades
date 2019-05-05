#ifndef HADES_UTIL_SHAREDGUARD_HPP
#define HADES_UTIL_SHAREDGUARD_HPP

#include <shared_mutex>

#include "hades/value_guard.hpp"
#include "hades/transactional.hpp"

// a thread safe wrapper for arbitary value types.
// support concurrent reads for values that do not change often
namespace hades {
	template<typename value, typename mutex = std::shared_mutex>
	class shared_guard : public value_guard<value, mutex>
	{
	public:
		shared_guard() = default;
		constexpr shared_guard(value desired) : value_guard(desired) {}
		shared_guard(const shared_guard& other) : value_guard(other) {}
		void operator=(const shared_guard&) = delete;

		using value_guard<value, mutex>::operator=;

		value get() const
		{
			std::shared_lock<mutex>(_mutex);
			return _value;
		}

		operator value() const
		{
			return get();
		}
	};

	template<typename T, typename U>
	struct is_transactional<shared_guard<T, U>> : std::true_type {};
}

#endif // !HADES_UTIL_SHAREDGUARD_HPP
