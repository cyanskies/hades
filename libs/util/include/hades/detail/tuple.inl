#include "hades/tuple.hpp"

#include <array>

//implementations of the tuple algorithms

namespace hades::detail
{
	template<tuple TupleTy, typename Func, typename ...Args, std::size_t ...Indicies>
	consteval bool is_tuple_for_each_invokable(std::index_sequence<Indicies...>) noexcept
	{
		using Tup = std::remove_reference_t<TupleTy>;
		return (std::is_invocable_v<Func, std::tuple_element_t<Indicies, Tup>, Args...> && ...)
			|| (std::is_invocable_v<Func, std::tuple_element_t<Indicies, Tup>, std::size_t, Args...> && ...);
	}

	template<tuple TupleTy, typename Func, typename ...Args, std::size_t ...Indicies>
	consteval bool is_tuple_for_each_nothrow_invokable(std::index_sequence<Indicies...>) noexcept
	{
		using Tup = std::remove_reference_t<TupleTy>;
		return ( (std::is_invocable_v<Func, std::tuple_element_t<Indicies, Tup>, Args...> && ...) &&
			(std::is_nothrow_invocable_v<Func, std::tuple_element_t<Indicies, Tup>, Args...> && ...) ) ||
			( (std::is_invocable_v<Func, std::tuple_element_t<Indicies, Tup>, std::size_t, Args...> && ...) &&
				(std::is_nothrow_invocable_v<Func, std::tuple_element_t<Indicies, Tup>, std::size_t, Args...> && ...));
	}

	template<tuple Tup, typename Func, typename ...Args, std::size_t ...Indicies>
		requires (is_tuple_for_each_invokable<Tup, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>()))
	constexpr void tuple_for_each_impl(Tup&& t, Func f, std::index_sequence<Indicies...>, Args&& ...args)
		noexcept(is_tuple_for_each_nothrow_invokable<Tup, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>()))
	{
		if constexpr((std::is_invocable_v<Func, std::tuple_element_t<Indicies, std::remove_reference_t<Tup>>, std::size_t, Args...> && ...))
		{
			std::apply([&]<typename ...Types>(Types&&... elements) {
				(static_cast<void>(std::invoke(f, std::forward<Types>(elements), Indicies, args...)), ...);
				return;
			}, std::forward<Tup>(t));
		}
		else
		{
			std::apply([&]<typename ...Types>(Types&&... elements) {
				(static_cast<void>(std::invoke(f, std::forward<Types>(elements), args...)), ...);
				return;
			}, std::forward<Tup>(t));
		}
		return;
	}

	template<tuple Tup, typename Func, typename ...Args, std::size_t ...Indicies>
		requires (is_tuple_for_each_invokable<Tup, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>()))
	consteval bool is_tuple_transform_homogeneous_result(std::index_sequence<Indicies...>) noexcept
	{
		// empty sets are homogenuous
		if constexpr (std::tuple_size_v<std::remove_reference_t<Tup>> == std::size_t{})
			return true;

		if constexpr ((std::is_invocable_v<Func, std::tuple_element_t<Indicies, std::remove_reference_t<Tup>>, std::size_t, Args...> && ...))
		{
			// passing the index to the called function
			using FirstResult = std::invoke_result_t<Func, std::tuple_element_t<0, std::remove_reference_t<Tup>>, std::size_t, Args...>;
			return (std::is_same_v<FirstResult, std::invoke_result_t<Func, std::tuple_element_t<Indicies, std::remove_reference_t<Tup>>, std::size_t, Args...>> && ...);
		}
		else
		{
			using FirstResult = std::invoke_result_t<Func, std::tuple_element_t<0, std::remove_reference_t<Tup>>, Args...>;
			return (std::is_same_v<FirstResult, std::invoke_result_t<Func, std::tuple_element_t<Indicies, std::remove_reference_t<Tup>>, Args...>> && ...);
		}
	}

	template<tuple Tup, typename Func, typename ...Args, std::size_t ...Indicies>
		requires (is_tuple_for_each_invokable<Tup, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>()))
	constexpr decltype(auto) tuple_transform_impl(Tup&& t, Func f, std::index_sequence<Indicies...>, Args&& ...args)
		noexcept(is_tuple_for_each_nothrow_invokable<Tup, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>()))
	{
		if constexpr ((std::is_invocable_v<Func, std::tuple_element_t<Indicies, std::remove_reference_t<Tup>>, std::size_t, Args...> && ...))
		{
			if constexpr (is_tuple_transform_homogeneous_result<Tup, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>()))
			{
				return std::apply([&]<typename ...Types>(Types&&... elements) {
					return std::array{
						std::invoke(f, std::forward<Types>(elements), Indicies, args...)...
					};
				}, std::forward<Tup>(t));
			}
			else
			{
				return std::apply([&]<typename ...Types>(Types&&... elements) {
					return std::tuple<std::invoke_result_t<Func, std::size_t, Types>...>{
						std::invoke(f, std::forward<Types>(elements), Indicies, args...)...
					};
				}, std::forward<Tup>(t));
			}
		}
		else
		{
			if constexpr (is_tuple_transform_homogeneous_result<Tup, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>()))
			{
				return std::apply([&]<typename ...Types>(Types&&... elements) {
					return std::array{
						std::invoke(f, std::forward<Types>(elements), args...)...
					};
				}, std::forward<Tup>(t));
			}
			else
			{
				return std::apply([&]<typename ...Types>(Types&&... elements) {
					return std::tuple<std::invoke_result_t<Func, Types>...>{
						std::invoke(f, std::forward<Types>(elements), args...)...
					};
				}, std::forward<Tup>(t));
			}
		}
	}

	// only calls the function for the correct index
	template<std::size_t Index, std::size_t End, tuple Tup, typename Func, typename... Args>
	inline constexpr void for_index_impl(Tup&& t, std::size_t i, Func&& f, Args&&... args)
		noexcept(is_tuple_for_each_nothrow_invokable<Tup, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>()))
	{
		if (Index == i)
			std::invoke(std::forward<Func>(f), std::get<Index>(t), std::forward<Args>(args)...);
		else if constexpr(Index < End)
			for_index_impl<Index + 1, End>(std::forward<Tup>(t), i, std::forward<Func>(f), std::forward<Args>(args)...);
		return;
	}
}
