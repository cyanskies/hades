#ifndef HADES_UTIL_STRONG_TYPEDEF_HPP
#define HADES_UTIL_STRONG_TYPEDEF_HPP

#include <algorithm>
#include <utility>

#include "hades/utility.hpp"

namespace hades
{
	//a strong typedef around a type
	// use by:
	// struct type_t {};
	// using type = strong_typedef<type_t, value_type>;
	template<typename Tag, typename T>
	struct strong_typedef
	{
		using tag_type = Tag;
		using value_type = T;

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

	private:
		value_type _value{};
	};

	template<typename T>
	struct is_strong_typedef : std::false_type {};

	template<typename Tag, typename T>
	struct is_strong_typedef<strong_typedef<Tag, T>> : std::true_type {};

	template<typename T>
	constexpr auto is_strong_typedef_v = is_strong_typedef<T>::value;

	template<typename Tag, typename T>
	constexpr bool operator==(const strong_typedef<Tag, T> &l, const strong_typedef<Tag, T> &r)
	{
		return static_cast<T>(l) == static_cast<T>(r);
	}

	template<typename Tag, typename T>
	inline constexpr bool operator!=(const strong_typedef<Tag, T> &l, const strong_typedef<Tag, T> &r)
	{
		return !(l == r);
	}

	template<typename Tag, typename T>
	constexpr bool operator<(const strong_typedef<Tag, T> &l, const strong_typedef<Tag, T> &r)
	{
		return static_cast<T>(l) < static_cast<T>(r);
	}

	template<typename Tag, typename T>
	string to_string(strong_typedef<Tag, T> e)
	{
		return to_string(static_cast<T&>(e));
	}

	template<typename Tag, typename T>
	strong_typedef<Tag, T> strong_typedef_from_string(std::string_view s)
	{
		return strong_typedef<Tag, T>{ from_string<T>(s) };
	}
}

#endif //!HADES_UTIL_STRONG_TYPEDEF_HPP
