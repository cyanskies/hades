#ifndef HADES_UTIL_STRONG_TYPEDEF_HPP
#define HADES_UTIL_STRONG_TYPEDEF_HPP

#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>

#include "hades/string.hpp"

//TODO: strong_typedef_traits<Tag> that strong_typedef can use for defines and constexpr values
// in order to toggle capabilities

namespace hades
{
	//a strong typedef around a type
	// use by:
	// struct type_t {};
	// using type = strong_typedef<type_t, value_type>;
	template<typename Tag>
	struct strong_typedef_traits
	{
		static constexpr auto arithmetic_operators = false;
	};

	template<typename Tag, typename T>
	struct strong_typedef
	{
		using tag_type = Tag;
		using value_type = T;
		static constexpr bool arithmetic_flag = strong_typedef_traits<Tag>::arithmetic_operators && std::is_arithmetic_v<T>;

		constexpr strong_typedef() noexcept(std::is_nothrow_default_constructible_v<T>) = default;
		explicit constexpr strong_typedef(value_type v) noexcept(std::is_nothrow_move_constructible_v<T>) 
			: _value{ std::move_if_noexcept(v) }
		{}

		explicit constexpr operator value_type() const noexcept
		{
			return _value;
		}

		explicit constexpr operator value_type&() noexcept
		{
			return _value;
		}

		friend void swap(strong_typedef &l, strong_typedef&r) noexcept
		{
			using std::swap;
			using V = value_type;
			return swap(static_cast<V&>(l), static_cast<V&>(r));
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U>::arithmetic_flag, int> = 0>
		constexpr strong_typedef operator+() const noexcept
		{
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U>::arithmetic_flag, int> = 0>
		constexpr strong_typedef operator-() const noexcept(noexcept(-_value))
		{
			return -_value;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U>::arithmetic_flag, int> = 0>
		constexpr strong_typedef &operator++() noexcept(noexcept(++_value))
		{
			++_value;
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U>::arithmetic_flag, int> = 0>
		constexpr strong_typedef operator++(int)
			noexcept(noexcept(operator++()) && std::is_nothrow_constructible_v<T, T>)
		{
			const auto tmp{ *this };
			operator++();
			return tmp;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U>::arithmetic_flag, int> = 0>
		constexpr strong_typedef &operator--() noexcept(noexcept(--_value))
		{
			--_value;
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U>::arithmetic_flag, int> = 0>
		constexpr strong_typedef operator--(int)
			noexcept(noexcept(operator--()) && std::is_nothrow_copy_constructible_v<T, T>)
		{
			const auto tmp{ *this };
			operator--();
			return tmp;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U>::arithmetic_flag, int> = 0>
		constexpr strong_typedef &operator+=(const strong_typedef &rhs)
			noexcept(noexcept(_value += rhs._value))
		{
			_value += rhs._value;
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U>::arithmetic_flag, int> = 0>
		constexpr strong_typedef &operator-=(const strong_typedef &rhs)
			noexcept(noexcept(_value -= rhs._value))
		{
			_value -= rhs._value;
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U>::arithmetic_flag, int> = 0>
		constexpr strong_typedef &operator*=(const strong_typedef &rhs)
			noexcept(noexcept(_value *= rhs._value))
		{
			_value *= rhs._value;
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U>::arithmetic_flag, int> = 0>
		constexpr strong_typedef &operator/=(const strong_typedef &rhs)
			noexcept(noexcept(_value /= rhs._value))
		{
			_value /= rhs._value;
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U>::arithmetic_flag, int> = 0>
		constexpr strong_typedef &operator%=(const strong_typedef &rhs)
			noexcept(noexcept(_value %= rhs._value))
		{
			_value %= rhs._value;
			return *this;
		}

	private:
		value_type _value;
	};

	template<typename T>
	struct is_strong_typedef : std::false_type {};

	template<typename Tag, typename T>
	struct is_strong_typedef<strong_typedef<Tag, T>> : std::true_type {};

	template<typename T>
	constexpr auto is_strong_typedef_v = is_strong_typedef<T>::value;

	template<typename T, typename = std::enable_if_t<is_strong_typedef_v<T>>>
	constexpr auto strong_typedef_operators_enabled_v = T::arithmetic_flag;

	template<typename Tag, typename T>
	inline constexpr bool operator==(const strong_typedef<Tag, T> &l, const strong_typedef<Tag, T> &r) noexcept
	{
		return static_cast<T>(l) == static_cast<T>(r);
	}

	template<typename Tag, typename T>
	inline constexpr bool operator!=(const strong_typedef<Tag, T> &l, const strong_typedef<Tag, T> &r) noexcept
	{
		return !(l == r);
	}

	template<typename Tag, typename T>
	constexpr bool operator<(const strong_typedef<Tag, T> &l, const strong_typedef<Tag, T> &r) noexcept
	{
		return static_cast<T>(l) < static_cast<T>(r);
	}

	template<typename Tag, typename T,
		typename = std::enable_if_t<strong_typedef_operators_enabled_v<strong_typedef<Tag, T>>>>
	constexpr strong_typedef<Tag, T> operator+(strong_typedef<Tag, T> lhs, const strong_typedef<Tag, T> &rhs)
		noexcept(noexcept(lhs += rhs))
	{
		lhs += rhs;
		return lhs;
	}

	template<typename Tag, typename T,
		typename = std::enable_if_t<strong_typedef_operators_enabled_v<strong_typedef<Tag, T>>>>
	constexpr strong_typedef<Tag, T> operator-(strong_typedef<Tag, T> lhs, const strong_typedef<Tag, T> &rhs)
		noexcept(noexcept(lhs -= rhs))
	{
		lhs -= rhs;
		return lhs;
	}

	template<typename Tag, typename T,
		typename = std::enable_if_t<strong_typedef_operators_enabled_v<strong_typedef<Tag, T>>>>
	constexpr strong_typedef<Tag, T> operator*(strong_typedef<Tag, T> lhs, const strong_typedef<Tag, T> &rhs)
		noexcept(noexcept(lhs *= rhs))
	{
		lhs *= rhs;
		return lhs;
	}

	template<typename Tag, typename T,
		typename = std::enable_if_t<strong_typedef_operators_enabled_v<strong_typedef<Tag, T>>>>
	constexpr strong_typedef<Tag, T> operator/(strong_typedef<Tag, T> lhs, const strong_typedef<Tag, T> &rhs)
		noexcept(noexcept(lhs /= rhs))
	{
		lhs /= rhs;
		return lhs;
	}

	template<typename Tag, typename T,
		typename = std::enable_if_t<strong_typedef_operators_enabled_v<strong_typedef<Tag, T>>>>
	constexpr strong_typedef<Tag, T> operator%(strong_typedef<Tag, T> lhs, const strong_typedef<Tag, T> &rhs)
		noexcept(noexcept(lhs %= rhs))
	{
		lhs %= rhs;
		return lhs;
	}

	template<typename Tag, typename T>
	constexpr strong_typedef<Tag, T> increment(strong_typedef<Tag, T>& v) noexcept(noexcept(++static_cast<T&>(v)))
	{
		return strong_typedef<Tag, T>{++static_cast<T&>(v)};
	}

	template<typename Tag, typename T>
	constexpr strong_typedef<Tag, T> post_increment(strong_typedef<Tag, T>& v) noexcept(noexcept(increment(v)))
	{
		const auto prev = v;
		increment(v);
		return prev;
	}

	template<typename Tag, typename T>
	constexpr strong_typedef<Tag, T> next(strong_typedef<Tag, T> v) noexcept(noexcept(increment(v)))
	{
		return increment(v);
	}

	template<typename Tag, typename T>
	T to_value(strong_typedef<Tag, T> e)
	{
		return static_cast<T>(e);
	}

	template<typename Tag, typename T>
	string to_string(strong_typedef<Tag, T> e)
	{
		return to_string(static_cast<T&>(e));
	}

	template<typename T, std::enable_if_t<is_strong_typedef_v<T>, int> = 0>
	T strong_typedef_from_string(std::string_view s)
	{
		return T{ from_string<typename T::value_type>(s) };
	}

	template<typename, typename = std::void_t<>>
	struct is_strong_typedef_hashable : std::false_type
	{};

	template<typename T>
	struct is_strong_typedef_hashable < T, std::void_t<decltype(std::hash<T>{}) >> : std::true_type
	{};
}

template <typename Tag, typename T>
struct std::hash<hades::strong_typedef<Tag, T>>
{
	static_assert(hades::is_strong_typedef_hashable<T>::value, "The type of this strong_typedef doesn't support hashing");
	using U = hades::strong_typedef<Tag, T>;

	size_t operator()(const U& key) const noexcept
	{
		const auto h = std::hash<typename U::value_type>{};
		return h(static_cast<typename U::value_type>(key));
	}
};

#endif //!HADES_UTIL_STRONG_TYPEDEF_HPP
