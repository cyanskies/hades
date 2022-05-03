#include "hades/for_each_tuple.hpp"

#include <tuple>

//implementations of the tuple algorithms

namespace hades::detail
{
	// returns true if func is nothrow for this type, or this type and an index param
	template<typename Func, typename T, typename ...Args>
	constexpr bool for_each_index_impl_is_noexcept() noexcept
	{
		if constexpr (std::is_invocable_v<Func, T, std::size_t, Args...>)
			return std::is_nothrow_invocable_v<Func, T, std::size_t, Args...>;
		else
			return std::is_nothrow_invocable_v<Func, T, Args...>;
	}

	template<std::size_t Index, typename Func, typename T, typename ...Args>
	inline constexpr auto for_each_index_impl(Func f, T& t, Args... args) noexcept(for_each_index_impl_is_noexcept<Func, T, Args...>())
	{
		if constexpr (std::is_invocable_v<Func, T, std::size_t, Args...>)
			return std::invoke(f, t, Index, args...);
		else
			return std::invoke(f, t, args...);
	}

	// returns true if the func is noexcept callable for all tuple elements
	template<typename Tuple, typename Func, typename... Args, std::size_t... Index>
	constexpr bool for_each_impl_is_noexcept(std::index_sequence<Index...>) noexcept
	{
		return (std::is_nothrow_invocable_v<Func, std::tuple_element_t<Index, Tuple>, Args...> && ...);
	}

	template<typename Tuple, typename Func, std::size_t... Index, typename ...Args>
	inline constexpr void for_each_impl(Tuple& t, Func f, std::index_sequence<Index...>, Args&&... args) 
		noexcept(for_each_impl_is_noexcept<Tuple, Func, Args...>(std::index_sequence<Index...>{}))
	{
		(for_each_index_impl<Index>(f, std::get<Index>(t), args...), ...);
		return;
	}

	template<typename Tuple, typename Func, std::size_t... Index, typename ...Args>
	inline decltype(auto) for_each_r_impl(Tuple& t, Func f, std::index_sequence<Index...>, Args&&... args)
		noexcept(for_each_impl_is_noexcept<Tuple, Func, Args...>(std::index_sequence<Index...>{}))
	{
		return std::array{ for_each_index_impl<Index>(f, std::get<Index>(t), args...)... };
	}

	template<typename Tuple, typename Func, typename... Args, std::size_t ...Index>
	constexpr bool for_index_is_noexcept(std::index_sequence<Index...>)
	{
		return ((!std::is_invocable_v<Func, std::tuple_element_t<Index, Tuple>, Tuple, Args...> ||
			std::is_nothrow_invocable_v<Func, std::tuple_element_t<Index, Tuple>, Tuple, Args...>) && ...);
	}

	// only calls the function for the correct index
	template<std::size_t Index, std::size_t End, typename Tuple, typename Func, typename... Args>
	inline constexpr void for_index_impl(Tuple& t, std::size_t i, Func&& f, Args&&... args) noexcept(
		for_index_is_noexcept<Tuple, Func, Args...>(std::make_index_sequence<End>()))
	{
		if (Index == i)
			std::invoke(std::forward<Func>(f), std::get<Index>(t), std::forward<Args>(args)...);
		else if constexpr(Index < End)
			for_index_impl<Index + 1, End>(t, i, std::forward<Func>(f), std::forward<Args>(args)...);
		return;
	}
}
