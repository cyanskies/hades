#ifndef HADES_UTIL_TUPLE_HPP
#define HADES_UTIL_TUPLE_HPP

#include <tuple>
#include <type_traits>

namespace hades
{
	// identify tuple-like types
	// these should support tuple_size, tuple_element and get<>
	template<typename T>
	concept tuple = requires
	{
		std::tuple_size<std::remove_reference_t<T>>::value;
	};
}

#include "hades/detail/tuple.inl"

namespace hades
{
	// ty Raymond @ old new thing
	// tuple_cat for uninstantiated type lists
	template<tuple ...Tuples>
	using tuple_type_cat_t = decltype(std::tuple_cat(std::declval<Tuples>()...));

	//Calls function on each element of tuple with 
	//that element and args... as functions argument
	//can optionally have std::size_t index as an argument after the element
	// function(elm, args...)
	// function(elm, std::size_t, args...)
	template<tuple Tup, typename Func, typename ...Args>
		requires (detail::is_tuple_for_each_invokable<Tup, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>()))
	constexpr void tuple_for_each(Tup&& t, Func&& function, Args&& ...args)
		noexcept(detail::is_tuple_for_each_nothrow_invokable<Tup, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>()))
	{
		return detail::tuple_for_each_impl(std::forward<Tup>(t), std::forward<Func>(function), std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>(), std::forward<Args>(args)...);
	}

	// tuple_transform
	// basically tuple_for_each, but also captures the invoked functions return values
	// if call calls to Func return the same type, the results will be returned in a std::array
	// otherwise they will be returned as a tuple
	template<tuple Tup, typename Func, typename ...Args>
		requires (detail::is_tuple_for_each_invokable<Tup, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>()))
	constexpr decltype(auto) tuple_transform(Tup&& t, Func&& function, Args&& ...args)
		noexcept(detail::is_tuple_for_each_nothrow_invokable<Tup, Func, Args...>(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>()))
	{
		return detail::tuple_transform_impl(std::forward<Tup>(t), std::forward<Func>(function), std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tup>>>(), std::forward<Args>(args)...);
	}

	//calls function on the tuple element at index
	//index can be specified at runtime.
	// function(elm, args...)
	template<tuple Tup, typename Func, typename ...Args>
	constexpr void tuple_index_invoke(Tup&& t, std::size_t index, Func&& function, Args&& ...args)
		noexcept(noexcept(detail::for_index_impl<0, std::tuple_size_v<std::remove_reference_t<Tup>>>(t, index, std::forward<Func>(function), std::forward<Args>(args)...)))
	{
		constexpr auto end = std::tuple_size_v<std::remove_reference_t<Tup>>;
		assert(index < end);
		detail::for_index_impl<0, end - 1>(std::forward<Tup>(t), index, std::forward<Func>(function), std::forward<Args>(args)...);
		return;
	}
}

#endif // !HADES_UTIL_FOR_EACH_TUPLE_HPP
