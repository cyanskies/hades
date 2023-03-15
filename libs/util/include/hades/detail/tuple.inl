#include "hades/tuple.hpp"

#include <array>

//implementations of the tuple algorithms

namespace hades::detail
{
	template<typename Tuple, typename Func, typename ...Args, std::size_t ...Indicies>
	consteval bool is_tuple_for_each_invokable(std::index_sequence<Indicies...>) noexcept
	{
		using Tup = std::remove_reference_t<Tuple>;
		return (std::is_invocable_v<Func, std::tuple_element_t<Indicies, Tup>, Args...> && ...)
			|| (std::is_invocable_v<Func, std::tuple_element_t<Indicies, Tup>, std::size_t, Args...> && ...);
	}

	template<typename Tuple, typename Func, typename ...Args, std::size_t ...Indicies>
	consteval bool is_tuple_for_each_nothrow_invokable(std::index_sequence<Indicies...>) noexcept
	{
		using Tup = std::remove_reference_t<Tuple>;
		return ( (std::is_invocable_v<Func, std::tuple_element_t<Indicies, Tup>, Args...> && ...) &&
			(std::is_nothrow_invocable_v<Func, std::tuple_element_t<Indicies, Tup>, Args...> && ...) ) ||
			( (std::is_invocable_v<Func, std::tuple_element_t<Indicies, Tup>, std::size_t, Args...> && ...) &&
				(std::is_nothrow_invocable_v<Func, std::tuple_element_t<Indicies, Tup>, std::size_t, Args...> && ...));
	}

	template<typename Tuple, typename Func, typename ...Args, std::size_t ...Indicies>
	constexpr void tuple_for_each_impl(Tuple&& t, Func f, std::index_sequence<Indicies...>, Args&& ...args) 
		noexcept(is_tuple_for_each_nothrow_invokable<Tuple, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>()))
		requires is_tuple<std::remove_reference_t<Tuple>> && (is_tuple_for_each_invokable<Tuple, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>()))
	{
		if constexpr((std::is_invocable_v<Func, std::tuple_element_t<Indicies, std::remove_reference_t<Tuple>>, std::size_t, Args...> && ...))
		{
			std::apply([&]<typename ...Types>(Types&&... elements) {
				(static_cast<void>(std::invoke(f, std::forward<Types>(elements), Indicies, args...)), ...);
				return;
			}, std::forward<Tuple>(t));
		}
		else
		{
			std::apply([&]<typename ...Types>(Types&&... elements) {
				(static_cast<void>(std::invoke(f, std::forward<Types>(elements), args...)), ...);
				return;
			}, std::forward<Tuple>(t));
		}
		return;
	}

	template<typename Tuple, typename Func, typename ...Args, std::size_t ...Indicies>
	consteval bool is_tuple_transform_homogeneous_result(std::index_sequence<Indicies...>) noexcept
		requires is_tuple<std::remove_reference_t<Tuple>> && (is_tuple_for_each_invokable<Tuple, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>()))
	{
		// empty sets are homogenuous
		if constexpr (std::tuple_size_v<std::remove_reference_t<Tuple>> == std::size_t{})
			return true;

		if constexpr ((std::is_invocable_v<Func, std::tuple_element_t<Indicies, std::remove_reference_t<Tuple>>, std::size_t, Args...> && ...))
		{
			// passing the index to the called function
			using FirstResult = std::invoke_result_t<Func, std::tuple_element_t<0, std::remove_reference_t<Tuple>>, std::size_t, Args...>;
			return (std::is_same_v<FirstResult, std::invoke_result_t<Func, std::tuple_element_t<Indicies, std::remove_reference_t<Tuple>>, std::size_t, Args...>> && ...);
		}
		else
		{
			using FirstResult = std::invoke_result_t<Func, std::tuple_element_t<0, std::remove_reference_t<Tuple>>, Args...>;
			return (std::is_same_v<FirstResult, std::invoke_result_t<Func, std::tuple_element_t<Indicies, std::remove_reference_t<Tuple>>, Args...>> && ...);
		}
	}

	template<typename Tuple, typename Func, typename ...Args, std::size_t ...Indicies>
	constexpr decltype(auto) tuple_transform_impl(Tuple&& t, Func f, std::index_sequence<Indicies...>, Args&& ...args)
		noexcept(is_tuple_for_each_nothrow_invokable<Tuple, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>()))
		requires is_tuple<std::remove_reference_t<Tuple>> && (is_tuple_for_each_invokable<Tuple, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>()))
	{
		if constexpr ((std::is_invocable_v<Func, std::tuple_element_t<Indicies, std::remove_reference_t<Tuple>>, std::size_t, Args...> && ...))
		{
			if constexpr (is_tuple_transform_homogeneous_result<Tuple, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>()))
			{
				return std::apply([&]<typename ...Types>(Types&&... elements) {
					return std::array{
						std::invoke(f, std::forward<Types>(elements), Indicies, args...)...
					};
				}, std::forward<Tuple>(t));
			}
			else
			{
				return std::apply([&]<typename ...Types>(Types&&... elements) {
					return std::tuple<std::invoke_result_t<Func, std::size_t, Types>...>{
						std::invoke(f, std::forward<Types>(elements), Indicies, args...)...
					};
				}, std::forward<Tuple>(t));
			}
		}
		else
		{
			if constexpr (is_tuple_transform_homogeneous_result<Tuple, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>()))
			{
				return std::apply([&]<typename ...Types>(Types&&... elements) {
					return std::array{
						std::invoke(f, std::forward<Types>(elements), args...)...
					};
				}, std::forward<Tuple>(t));
			}
			else
			{
				return std::apply([&]<typename ...Types>(Types&&... elements) {
					return std::tuple<std::invoke_result_t<Func, Types>...>{
						std::invoke(f, std::forward<Types>(elements), args...)...
					};
				}, std::forward<Tuple>(t));
			}
		}
	}

	// only calls the function for the correct index
	template<std::size_t Index, std::size_t End, typename Tuple, typename Func, typename... Args>
	inline constexpr void for_index_impl(Tuple&& t, std::size_t i, Func&& f, Args&&... args)
		noexcept(is_tuple_for_each_nothrow_invokable<Tuple, Func, Args...>(std::make_index_sequence<End>()))
	{
		if (Index == i)
			std::invoke(std::forward<Func>(f), std::get<Index>(t), std::forward<Args>(args)...);
		else if constexpr(Index < End)
			for_index_impl<Index + 1, End>(std::forward<Tuple>(t), i, std::forward<Func>(f), std::forward<Args>(args)...);
		return;
	}
}
