#ifndef HADES_UTIL_VALUEGUARD_HPP
#define HADES_UTIL_VALUEGUARD_HPP

#include <mutex>
#include <utility>

#include "hades/transactional.hpp"

// a thread safe wrapper for arbitary value types.
namespace hades {
	template<typename value, typename mutex = std::mutex>
	class value_guard
	{
	public:
		using exchange_token = std::unique_lock<mutex>;
		using lock_return = std::tuple<bool, exchange_token>;
		using value_type = value;

		value_guard() = default;

		value_guard(value desired) 
			noexcept(std::is_nothrow_copy_constructible_v<value>)
			: _value(desired) 
		{}

		value_guard(const value_guard& other) : _value(other.load()) {}
		virtual ~value_guard() = default;

		template<typename T, typename = std::enable_if_t<std::is_same_v<std::decay_t<T>, value>>>
		void operator=(T &&desired)
		{
			std::lock_guard<mutex> lk(_mutex);
			_value = std::forward<T>(desired);
		}

		void operator=(const value_guard&) = delete;

		value load() const
		{
			std::lock_guard<mutex> lk(_mutex);
			return _value;
		}

		value get() const
		{
			return load();
		}

		operator value() const
		{
			return load();
		}

		template<typename T, typename = std::enable_if_t<std::is_same_v<std::decay_t<T>, value>>>
		bool compare_exchange(const value &expected, T &&desired)
		{
			std::lock_guard<mutex> lk(_mutex);
			if (_value == expected)
			{
				_value = std::forward<T>(desired);
				return true;
			}

			return false;
		}

		//returns true if the lock was granted, the lock is only granted if expected == the current stored value
		lock_return exchange_lock(const value &expected) const
		{
			exchange_token lk{_mutex};
			bool success{ _value == expected };

			if(!success)
				lk.unlock();

			return std::make_tuple(success, std::move(lk));
		}

		//releases the lock that is already held, without commiting the changes
		void exchange_release(exchange_token token) const noexcept
		{
			assert(token.owns_lock());
			assert(*token.mutex() == _mutex);
		}

		//releases the lock after exchanging the value
		template<typename T, typename = std::enable_if_t<std::is_same_v<std::decay_t<T>, value>>>
		void exchange_resolve(T &&desired, exchange_token &&token)
		{
			assert(token.owns_lock());
			assert(*token.mutex() == _mutex);
			_value = std::forward<T>(desired);
		}

		constexpr bool is_lock_free() const noexcept { return false; }
		static constexpr bool is_always_lock_free = false;

	protected:
		mutable mutex _mutex;
		value _value;
	};

	template<typename T, typename U>
	struct is_transactional<value_guard<T, U>> : std::true_type {};
}

#endif // !HADES_UTIL_VALUEGUARD_HPP
