#ifndef HADES_UTIL_FOR_EACH_TUPLE_HPP
#define HADES_UTIL_FOR_EACH_TUPLE_HPP

#include <tuple>
#include <vector>

// TODO: rename header tuple tools

#include "hades/detail/for_each_tuple.inl"

namespace hades
{
	// ty Raymond @ old new thing
	// tuple_cat for uninstantiated type lists
	template<typename... Tuples>
	using tuple_type_cat_t = decltype(std::tuple_cat(std::declval<Tuples>()...));

	//Calls function on each element of tuple with 
	//that element and args... as functions argument
	//can optionally have std::size_t index as an argument after the element
	// function(elm, args...)
	// function(elm, std::size_t, args...)
	template<typename Tuple, typename Func, typename ...Args>
	void for_each_tuple(Tuple& t, Func function, Args&& ...args)
		noexcept(noexcept(detail::for_each_impl(t, function, std::make_index_sequence<std::tuple_size_v<Tuple>>{}, std::forward<Args>(args)...)))
		requires is_tuple<Tuple>
	{
		detail::for_each_impl(t, function, std::make_index_sequence<std::tuple_size_v<Tuple>>{}, std::forward<Args>(args)...);
		return;
	}

	//Same as above, returns an array containing the return values of
	//each function call, function must return the same type for all elements
	template<typename Tuple, typename Func, typename ...Args>
	decltype(auto) for_each_tuple_r(Tuple& t, Func f, Args&& ...args)
		noexcept(noexcept(detail::for_each_r_impl(t, f, std::make_index_sequence<std::tuple_size_v<Tuple>>{}, std::forward<Args>(args)...)))
		requires is_tuple<Tuple>
	{
		return detail::for_each_r_impl(t, f, std::make_index_sequence<std::tuple_size_v<Tuple>>{}, std::forward<Args>(args)...);
	}

	//calls function on the tuple element at index
	//index can be specified at runtime.
	// function(elm, args...)
	template<typename Tuple, typename Func, typename ...Args>
	constexpr void for_index_tuple(Tuple& t, std::size_t index, Func&& function, Args&& ...args)
		noexcept(noexcept(detail::for_index_impl<0, std::tuple_size_v<Tuple>>(t, index, std::forward<Func>(function), std::forward<Args>(args)...)))
		requires is_tuple<Tuple>
	{
		constexpr auto s = std::tuple_size_v<Tuple>;
		assert(index < s);
		detail::for_index_impl<0, s - 1>(t, index, std::forward<Func>(function), std::forward<Args>(args)...);
		return;
	}
}

#endif // !HADES_UTIL_FOR_EACH_TUPLE_HPP
