#ifndef HADES_UTIL_SPINLOCK_HPP
#define HADES_UTIL_SPINLOCK_HPP

#include <atomic>
#include <cassert>
#include <limits>
#include <type_traits>

namespace hades
{
	// Mutex
	class spinlock
	{
	public:
		void lock() noexcept
		{
			while (_locked.test_and_set(std::memory_order_relaxed))
				continue;
		}

		bool try_lock() noexcept
		{
			return !_locked.test_and_set(std::memory_order_relaxed);
		}

		void unlock() noexcept
		{
			assert(_locked.test_and_set());
			_locked.clear(std::memory_order_relaxed);
		}

	private:
		std::atomic_flag _locked = ATOMIC_FLAG_INIT;
	};

	// SharedMutex
	class shared_spinlock
	{
	public:
		void lock() noexcept
		{
			auto desired = unlocked;
			while (!_count.compare_exchange_weak(desired, exclusive_lock,
				std::memory_order_release, std::memory_order_acquire))
				desired = unlocked;
		}

		bool try_lock() noexcept
		{
			auto desired = unlocked;
			return _count.compare_exchange_strong(desired, exclusive_lock,
				std::memory_order_relaxed, std::memory_order_relaxed);
		}

		void unlock() noexcept
		{
			assert(_count.load() == exclusive_lock);
			_count.store(unlocked, std::memory_order_relaxed);
		}

		void lock_shared() noexcept
		{
			auto old = _count.load(std::memory_order_acquire);
			do
			{
				if (old == exclusive_lock
					|| old == shared_max)
					continue;
			} while (!_count.compare_exchange_weak(old, old + 1,
				std::memory_order_release, std::memory_order_acquire));
		}

		bool try_lock_shared() noexcept
		{
			auto old = _count.load(std::memory_order_acquire);
			if (old == exclusive_lock
				|| old == shared_max)
				return false;

			return _count.compare_exchange_strong(old, old + 1,
				std::memory_order_release, std::memory_order_relaxed);
		}

		void unlock_shared() noexcept
		{
			auto val = _count.fetch_sub(1, std::memory_order_relaxed);
			assert(val != exclusive_lock);
		}

	private:
		using type = unsigned char;
		static constexpr auto exclusive_lock = type{ 0 };
		static constexpr auto unlocked = type{ 1 };
		static constexpr auto shared_max = std::numeric_limits<type>::max();

		std::atomic<type> _count{ unlocked };
		static_assert(std::atomic<type>::is_always_lock_free);
	};

	template<typename T>
	struct lock_free_mutex : std::false_type {};
	template<>
	struct lock_free_mutex<spinlock> : std::true_type {};
	template<>
	struct lock_free_mutex<shared_spinlock> : std::true_type {};

	template<typename T>
	constexpr auto lock_free_mutex_v = lock_free_mutex<T>::value;
}

#endif // !HADES_UTIL_SPINLOCK_HPP