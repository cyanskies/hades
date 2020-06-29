#include "hades/for_each_tuple.hpp"

#include <tuple>

namespace hades::detail
{
	template<typename Func, typename T, typename ...Args>
	constexpr bool for_each_index_impl_noexcept() noexcept
	{
		if constexpr (std::is_invocable_v<Func, T, std::size_t, Args...>)
			return std::is_nothrow_invocable_v<Func, T, std::size_t, Args...>;
		else
			return std::is_nothrow_invocable_v<Func, T, Args...>;
	}

	template<std::size_t Index, typename Func, typename T, typename ...Args>
	inline constexpr auto for_each_index_impl(Func f, T& t, Args... args) noexcept(for_each_index_impl_noexcept<Func, T, Args...>())
	{
		if constexpr (std::is_invocable_v<Func, T, std::size_t, Args...>)
			return std::invoke(f, t, Index, args...);
		else
			return std::invoke(f, t, args...);
	}

	template<typename Tuple, typename Func, typename... Args, std::size_t... Index>
	constexpr bool for_each_impl_noexcept(std::index_sequence<Index...>) noexcept
	{
		return (std::is_nothrow_invocable_v<Func, std::tuple_element_t<Index, Tuple>, Args...> && ...);
	}

	template<typename Tuple, typename Func, std::size_t... Index, typename ...Args>
	inline constexpr void for_each_impl(Tuple& t, Func f, std::index_sequence<Index...> i, Args... args) 
		noexcept(for_each_impl_noexcept<Tuple, Func, Args...>(i))
	{
		(for_each_index_impl<Index>(f, std::get<Index>(t), args...), ...);
		return;
	}

	template<typename... Ts, typename Func, std::size_t... Index, typename ...Args>
	inline decltype(auto) for_each_r_impl(std::tuple<Ts...>& t, Func f, std::index_sequence<Index...> i, Args... args)
		noexcept(for_each_impl_noexcept<std::tuple<Ts...>, Func, Args...>(i))
	{
		return std::array{ for_each_index_impl<Index>(f, std::get<Index>(t), args...)... };
	}

	template<typename... Ts, typename Func, std::size_t... Index, typename ...Args>
	inline decltype(auto) for_each_r_impl(const std::tuple<Ts...>& t, Func f, std::index_sequence<Index...> i, Args... args)
		noexcept(for_each_impl_noexcept<std::tuple<Ts...>, Func, Args...>(i))
	{
		return std::array{ for_each_index_impl<Index>(f, std::get<Index>(t), args...)... };
	}

	template<std::size_t Index, typename Func, typename T, typename ...Args>
	inline constexpr void for_index_impl2(T& t, std::size_t i, Func f, Args... args)
		noexcept(std::is_nothrow_invocable_v<Func, T, Args...>)
	{
		if (Index == i)
			std::invoke(f, t, args...);
		return;
	}

	template<typename Func, typename Tuple, std::size_t... Index, typename... Args>
	inline constexpr void for_index_impl(Tuple &t, std::size_t i, std::index_sequence<Index...>, Func f, Args... args)
	{
		(for_index_impl2<Index>(std::get<Index>(t), i, f, args...), ...);
		return;
	}
}
