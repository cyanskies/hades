#ifndef HADES_UTIL_FOR_EACH_TUPLE_HPP
#define HADES_UTIL_FOR_EACH_TUPLE_HPP

#include <tuple>
#include <vector>

namespace hades
{
	//Calls function on each element of tuple with 
	//that element and args... as functions argument
	//can optionally have std::size_t index as an argument after the element
	// function(elm, args...)
	// function(elm, std::size_t, args...)
	template<typename Tuple, typename Func, typename ...Args>
	void for_each_tuple(Tuple &tuple, Func function, Args... args)
		noexcept(noexcept(detail::for_each_worker < std::size_t{ 0u } > (t, f, args...)));

	//Same as above, returns a vector containing the return values of
	//each function call, function must return the same type for all elements
	template<typename Tuple, typename Func, typename...Args>
    decltype(auto) for_each_tuple_r(Tuple &tuple, Func function, Args... args)
		noexcept(noexcept(detail::for_each_worker_r < std::size_t{ 0u } > (t, f, args...)));

	//calls function on the tuple element at index
	//index can be specified at runtime.
	// function(elm)
	template<typename Func, typename Tuple, typename ...Args>
	void for_index_tuple(Tuple &tuple, std::size_t index, Func function, Args... args)
		noexcept(noexcept(detail::for_index_worker <std::size_t{ 0u } > (t, i, f, args...)));
}

#include "hades/detail/for_each_tuple.inl"

#endif // !HADES_UTIL_FOR_EACH_TUPLE_HPP
