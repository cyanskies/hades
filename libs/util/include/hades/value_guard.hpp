#ifndef HADES_UTIL_VALUEGUARD_HPP
#define HADES_UTIL_VALUEGUARD_HPP

#include <mutex>

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

		void operator=(value desired)
		{
			std::lock_guard<mutex> lk(_mutex);
			_value = std::move(desired);
		}

		void operator=(const value_guard&) = delete;

		value load() const
		{
			std::lock_guard<mutex> lk(_mutex);
			return _value;
		}

		operator value() const
		{
			return load();
		}

		bool compare_exchange(const value &expected, value desired)
		{
			std::lock_guard<mutex> lk(_mutex);
			if (_value == expected)
			{
				_value = std::move(desired);
				return true;
			}

			return false;
		}

		//returns true if the lock was granted, the lock is only granted if expected == the current stored value
		lock_return exchange_lock(value expected) const
		{
			exchange_token lk{_mutex};
			bool success{ _value == expected };

			if(!success)
				lk.unlock();

			return std::make_tuple(success, std::move(lk));
		}
		//releases the lock that is already held, without commiting the changes
		void exchange_release(exchange_token &&token) const
		{
			assert(token.owns_lock());
			assert(token.mutex == _mutex);
		}
		//releases the lock after exchanging the value
		void exchange_resolve(value desired, exchange_token &&token)
		{
			assert(token.owns_lock());
			assert(token.mutex == _mutex);
			_value = desired;
		}

		constexpr bool is_lock_free() const noexcept { return false; }
		static constexpr bool is_always_lock_free = false;

	protected:
		mutable mutex _mutex;
		value _value;
	};
}

#endif // !HADES_UTIL_VALUEGUARD_HPP
