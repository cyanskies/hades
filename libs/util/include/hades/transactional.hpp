#ifndef HADES_TRANSACTIONAL_HPP
#define HADES_TRANSACTIONAL_HPP

#include <type_traits>

// Definitions to aid in using transactional types

namespace hades
{
	template<typename T>
	struct is_transactional : std::false_type {};

	template<typename T>
	constexpr auto is_transactional_v = is_transactional<T>::value;

	namespace transactional
	{
		template<typename T, template<typename> typename U>
		T get(const U<T> &u)
		{
			static_assert<is_transactional<U<T>>>;
			return u.get();
		}

		template<typename Key, typename Ty, template<typename, typename> typename U>
		Ty get(const Key &k, const U<Key, Ty>& u)
		{
			static_assert<is_transactional<U<Key, Ty>>>;
			return u.get(k);
		}

		template<typename T, template<typename> typename U>
		void compare_exchange(const U<T>& u, const T &current, T &&desired)
		{
			static_assert<is_transactional<U<T>>>;
			u.compare_exchange(current, std::forward(desired));
		}

		template<typename T, template<typename> typename U>
		typename U<T>::lock_return exchange_lock(const T& expected, const U<T>& u)
		{
			static_assert<is_transactional<U<T>>>;
			return u.exchange_lock(expected);
		}

		template<typename K, typename T, template<typename, typename> typename U>
		typename U<K, T>::lock_return exchange_lock(const K& key, const T& expected, const U<K, T>& u)
		{
			static_assert<is_transactional<U<K, T>>>;
			return u.exchange_lock(key, expected);
		}

		template<typename T, template<typename> typename U>
		void exchange_release(const U<T>& u, typename U<T>::exchange_token l)
		{
			static_assert<is_transactional<U<T>>>;
			u.exchange_release(std::move(l));
		}

		template<typename K, typename T, template<typename, typename> typename U>
		void exchange_lock(const K& key, const U<K, T>& u, typename U<K, T>::exchange_token l)
		{
			static_assert<is_transactional<U<K, T>>>;
			return u.exchange_release(key, std::move(l));
		}

		template<typename T, template<typename> typename U>
		void exchange_resolve(const U<T>& u, T &&value, typename U<T>::exchange_token l)
		{
			static_assert<is_transactional<U<T>>>;
			u.exchange_resolve(std::forward(value), std::move(l));
		}

		template<typename K, typename T, template<typename, typename> typename U>
		void exchange_resolve(const K& key, const U<K, T>& u, T &&value, typename U<K, T>::exchange_token l)
		{
			static_assert<is_transactional<U<K, T>>>;
			return u.exchange_resolve(key, std::forward(value), std::move(l));
		}
	}
}

#endif // !HADES_UTIL_TRANSACTIONAL_HPP
