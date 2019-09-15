#ifndef HADES_UTIL_STRONG_TYPEDEF_HPP
#define HADES_UTIL_STRONG_TYPEDEF_HPP

#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>

#include "hades/utility.hpp"

//TODO: strong_typedef can accept a struct type with defines and constexpr values
// in order to toggle capabilities

namespace hades
{
	//a strong typedef around a type
	// use by:
	// struct type_t {};
	// using type = strong_typedef<type_t, value_type>;
	template<typename Tag, typename T, bool ArithmeticOperators = false>
	struct strong_typedef
	{
		using tag_type = Tag;
		using value_type = T;
		static constexpr bool arithmatic_flag = ArithmeticOperators && std::is_arithmetic_v<T>;

		//TODO: enable_if is_default_constructable
		constexpr strong_typedef() noexcept = default;
		//TODO: enable_if is_move_constructable or is_copy_constructable
		explicit constexpr strong_typedef(value_type v) noexcept(std::is_nothrow_move_constructible_v<T>) 
			: _value{ std::move_if_noexcept(v) }
		{}

		~strong_typedef() noexcept = default;

		constexpr strong_typedef(const strong_typedef&) noexcept = default;
		constexpr strong_typedef(strong_typedef&&) noexcept = default;

		constexpr strong_typedef &operator=(const strong_typedef&) noexcept = default;
		constexpr strong_typedef &operator=(strong_typedef&&) noexcept = default;

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

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U, ArithmeticOperators>::arithmatic_flag, int> = 0>
		constexpr strong_typedef operator+() const noexcept
		{
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U, ArithmeticOperators>::arithmatic_flag, int> = 0>
		constexpr strong_typedef operator-() const noexcept(noexcept(-_value))
		{
			return -_value;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U, ArithmeticOperators>::arithmatic_flag, int> = 0>
		constexpr strong_typedef &operator++() noexcept(noexcept(++_value))
		{
			++_value;
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U, ArithmeticOperators>::arithmatic_flag, int> = 0>
		constexpr strong_typedef operator++(int)
			noexcept(noexcept(operator++()) && std::is_nothrow_constructible_v<T, T>)
		{
			const auto tmp{ *this };
			operator++();
			return tmp;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U, ArithmeticOperators>::arithmatic_flag, int> = 0>
		constexpr strong_typedef &operator--() noexcept(noexcept(--_value))
		{
			--_value;
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U, ArithmeticOperators>::arithmatic_flag, int> = 0>
		constexpr strong_typedef operator--(int)
			noexcept(noexcept(operator--()) && std::is_nothrow_copy_constructible_v<T, T>)
		{
			const auto tmp{ *this };
			operator--();
			return tmp;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U, ArithmeticOperators>::arithmatic_flag, int> = 0>
		constexpr strong_typedef &operator+=(const strong_typedef &rhs)
			noexcept(noexcept(_value += rhs._value))
		{
			_value += rhs._value;
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U, ArithmeticOperators>::arithmatic_flag, int> = 0>
		constexpr strong_typedef &operator-=(const strong_typedef &rhs)
			noexcept(noexcept(_value -= rhs._value))
		{
			_value -= rhs._value;
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U, ArithmeticOperators>::arithmatic_flag, int> = 0>
		constexpr strong_typedef &operator*=(const strong_typedef &rhs)
			noexcept(noexcept(_value *= rhs._value))
		{
			_value *= rhs._value;
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U, ArithmeticOperators>::arithmatic_flag, int> = 0>
		constexpr strong_typedef &operator/=(const strong_typedef &rhs)
			noexcept(noexcept(_value /= rhs._value))
		{
			_value /= rhs._value;
			return *this;
		}

		template<typename U = T, std::enable_if_t<strong_typedef<Tag, U, ArithmeticOperators>::arithmatic_flag, int> = 0>
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

	template<typename Tag, typename T, bool ArithmeticOp>
	struct is_strong_typedef<strong_typedef<Tag, T, ArithmeticOp>> : std::true_type {};

	template<typename T>
	constexpr auto is_strong_typedef_v = is_strong_typedef<T>::value;

	template<typename T, typename = std::enable_if_t<is_strong_typedef_v<T>>>
	constexpr auto strong_typedef_operators_enabled_v = T::arithmetic_flag;

	template<typename Tag, typename T, bool B>
	inline constexpr bool operator==(const strong_typedef<Tag, T, B> &l, const strong_typedef<Tag, T, B> &r) noexcept
	{
		return static_cast<T>(l) == static_cast<T>(r);
	}

	template<typename Tag, typename T, bool B>
	inline constexpr bool operator!=(const strong_typedef<Tag, T, B> &l, const strong_typedef<Tag, T, B> &r) noexcept
	{
		return !(l == r);
	}

	template<typename Tag, typename T, bool B>
	constexpr bool operator<(const strong_typedef<Tag, T, B> &l, const strong_typedef<Tag, T, B> &r) noexcept
	{
		return static_cast<T>(l) < static_cast<T>(r);
	}

	template<typename Tag, typename T, bool B,
		typename = std::enable_if_t<strong_typedef_operators_enabled_v<strong_typedef<Tag, T, B>>>>
	constexpr strong_typedef<Tag, T> operator+(strong_typedef<Tag, T, B> lhs, const strong_typedef<Tag, T, B> &rhs)
		noexcept(noexcept(lhs += rhs))
	{
		lhs += rhs;
		return lhs;
	}

	template<typename Tag, typename T, bool B,
		typename = std::enable_if_t<strong_typedef_operators_enabled_v<strong_typedef<Tag, T, B>>>>
	constexpr strong_typedef<Tag, T> operator-(strong_typedef<Tag, T, B> lhs, const strong_typedef<Tag, T, B> &rhs)
		noexcept(noexcept(lhs -= rhs))
	{
		lhs -= rhs;
		return lhs;
	}

	template<typename Tag, typename T, bool B, 
		typename = std::enable_if_t<strong_typedef_operators_enabled_v<strong_typedef<Tag, T, B>>>>
	constexpr strong_typedef<Tag, T> operator*(strong_typedef<Tag, T, B> lhs, const strong_typedef<Tag, T, B> &rhs)
		noexcept(noexcept(lhs *= rhs))
	{
		lhs *= rhs;
		return lhs;
	}

	template<typename Tag, typename T, bool B,
		typename = std::enable_if_t<strong_typedef_operators_enabled_v<strong_typedef<Tag, T, B>>>>
	constexpr strong_typedef<Tag, T> operator/(strong_typedef<Tag, T, B> lhs, const strong_typedef<Tag, T, B> &rhs)
		noexcept(noexcept(lhs /= rhs))
	{
		lhs /= rhs;
		return lhs;
	}

	template<typename Tag, typename T, bool B,
		typename = std::enable_if_t<strong_typedef_operators_enabled_v<strong_typedef<Tag, T, B>>>>
	constexpr strong_typedef<Tag, T> operator%(strong_typedef<Tag, T, B> lhs, const strong_typedef<Tag, T, B> &rhs)
		noexcept(noexcept(lhs %= rhs))
	{
		lhs %= rhs;
		return lhs;
	}

	template<typename Tag, typename T, bool B>
	constexpr strong_typedef<Tag, T, B> increment(strong_typedef<Tag, T, B>& v) noexcept(noexcept(++static_cast<T&>(v)))
	{
		return strong_typedef<Tag, T, B>{++static_cast<T&>(v)};
	}

	template<typename Tag, typename T, bool B>
	constexpr strong_typedef<Tag, T, B> post_increment(strong_typedef<Tag, T, B>& v) noexcept(noexcept(increment(v)))
	{
		const auto prev = v;
		increment(v);
		return prev;
	}

	template<typename Tag, typename T, bool B>
	constexpr strong_typedef<Tag, T, B> next(strong_typedef<Tag, T, B> v) noexcept(noexcept(increment(v)))
	{
		return increment(v);
	}

	template<typename Tag, typename T, bool B>
	T to_value(strong_typedef<Tag, T, B> e)
	{
		return static_cast<T>(e);
	}

	template<typename Tag, typename T, bool B>
	string to_string(strong_typedef<Tag, T, B> e)
	{
		return to_string(static_cast<T&>(e));
	}

	template<typename Tag, typename T, bool B>
	strong_typedef<Tag, T, B> strong_typedef_from_string(std::string_view s)
	{
		return strong_typedef<Tag, T, B>{ from_string<T>(s) };
	}

	template<typename, typename = std::void_t<>>
	struct is_strong_typedef_hashable : std::false_type
	{};

	template<typename T>
	struct is_strong_typedef_hashable < T, std::void_t<decltype(std::hash<T>{}) >> : std::true_type
	{};
}

namespace std
{
	template <typename Tag, typename T, bool Arith>
	struct hash<hades::strong_typedef<Tag, T, Arith>>
	{
		static_assert(hades::is_strong_typedef_hashable<T>::value, "The type of this strong_typedef doesn't support hashing");
		using U = hades::strong_typedef<Tag, T, Arith>;

		size_t operator()(const U& key) const noexcept
		{
			const auto h = std::hash<U::value_type>{};
			return h(static_cast<U::value_type>(key));
		}
	};
}

#endif //!HADES_UTIL_STRONG_TYPEDEF_HPP
