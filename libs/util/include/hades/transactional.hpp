#ifndef HADES_TRANSACTIONAL_HPP
#define HADES_TRANSACTIONAL_HPP

#include <stdexcept>
#include <type_traits>

// Definitions to aid in using transactional types

namespace hades
{
	//TODO: move hades exceptions to util and make this base class from hades::runtime_error
	class transactional_error : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	template<typename T>
	struct is_transactional : std::false_type {};

	template<typename T>
	constexpr auto is_transactional_v = is_transactional<T>::value;

	//TODO: update transactional classes to support using generation ids to avoid large comparisons
	// such functions should have _gen postfix, and return exchange_token_gen tokens

	namespace transactional
	{
		//functions for guards
		template<typename T, template<typename> typename U>
		T get(const U<T> &u) noexcept(noexcept(u.get()))
		{
			static_assert(is_transactional_v<U<T>>);
			return u.get();
		}

		template<typename T, template<typename> typename U>
		void compare_exchange(U<T>& u, const T& current, T&& desired)
			noexcept(noexcept(u.compare_exchange(current, std::forward<T>(desired))))
		{
			static_assert(is_transactional_v<U<T>>);
			u.compare_exchange(current, std::forward<T>(desired));
		}

		template<typename T, template<typename> typename U>
		typename U<T>::lock_return exchange_lock(const T& expected, const U<T>& u)
			noexcept(noexcept(u.exchange_lock(expected)))
		{
			static_assert(is_transactional_v<U<T>>);
			return u.exchange_lock(expected);
		}

		template<typename T, template<typename> typename U>
		void exchange_release(const U<T>& u, typename U<T>::exchange_token l)
			noexcept(noexcept(u.exchange_release(std::move(l))))
		{
			static_assert(is_transactional_v<U<T>>);
			u.exchange_release(std::move(l));
		}

		template<typename T, template<typename> typename U>
		void exchange_resolve(U<T>& u, T&& value, typename U<T>::exchange_token l)
			noexcept(noexcept(u.exchange_resolve(std::forward<T>(value), std::move(l))))
		{
			static_assert(is_transactional_v<U<T>>);
			u.exchange_resolve(std::forward<T>(value), std::move(l));
		}

		//functions for maps
		template<typename Key, typename Ty, template<typename, typename> typename U>
		Ty get(const Key &k, const U<Key, Ty>& u) noexcept(noexcept(u.get(k)))
		{
			static_assert(is_transactional_v<U<Key, Ty>>);
			return u.get(k);
		}

		template<typename K, typename T, template<typename, typename> typename U>
		typename U<K, T>::lock_return exchange_lock(const K& key, const T& expected, const U<K, T>& u)
			noexcept(noexcept(u.exchange_lock(key, expected)))
		{
			static_assert(is_transactional_v<U<K, T>>);
			return u.exchange_lock(key, expected);
		}

		template<typename K, typename T, template<typename, typename> typename U>
		void exchange_release(const K& key, const U<K, T>& u, typename U<K, T>::exchange_token l)
			noexcept(noexcept(u.exchange_release(key, std::move(l))))
		{
			static_assert(is_transactional_v<U<K, T>>);
			return u.exchange_release(key, std::move(l));
		}

		template<typename K, typename T, template<typename, typename> typename U>
		void exchange_resolve(const K& key, U<K, T>& u, T &&value, typename U<K, T>::exchange_token l)
			noexcept(noexcept(u.exchange_resolve(key, std::forward<T>(value), std::move(l))))
		{
			static_assert(is_transactional_v<U<K, T>>);
			return u.exchange_resolve(key, std::forward<T>(value), std::move(l));
		}

		//functions for any_map
		template<typename Ty, typename Key, template<typename> typename U>
		Ty get(const Key& k, const U<Key>& u) noexcept(noexcept(u.get<Ty>(k)))
		{
			static_assert(is_transactional_v<U<Key>>);
			return u.get<Ty>(k);
		}

		template<typename T, typename K, template<typename> typename U>
		typename U<K>::lock_return exchange_lock(const K& key, const T& expected, const U<K>& u)
			noexcept(noexcept(u.exchange_lock(key, expected)))
		{
			static_assert(is_transactional_v<U<K>>);
			return u.exchange_lock(key, expected);
		}

		template<typename K, template<typename> typename U>
		void exchange_release(const K& key, const U<K>& u, typename U<K>::exchange_token l)
			noexcept(noexcept(u.exchange_release(key, std::move(l))))
		{
			static_assert(is_transactional_v<U<K>>);
			return u.exchange_release(key, std::move(l));
		}

		template<typename T, typename K, template<typename> typename U>
		void exchange_resolve(const K& key, U<K>& u, T&& value, typename U<K>::exchange_token l)
			noexcept(noexcept(u.exchange_resolve(key, std::forward<T>(value), std::move(l))))
		{
			static_assert(is_transactional_v<U<K>>);
			return u.exchange_resolve(key, std::forward<T>(value), std::move(l));
		}
	}
}

#endif // !HADES_UTIL_TRANSACTIONAL_HPP
