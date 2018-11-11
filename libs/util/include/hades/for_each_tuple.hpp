#ifndef HADES_UTIL_FOR_EACH_TUPLE_HPP
#define HADES_UTIL_FOR_EACH_TUPLE_HPP

#include <tuple>

namespace hades
{
	template<typename Func, typename ...Ts>
	void for_each_tuple(const std::tuple<Ts...> &tuple, Func function);

	template<typename Func, typename ...Ts>
	void for_each_tuple(std::tuple<Ts...> &tuple, Func function);

	template<typename Func, typename ...Ts>
	void for_index_tuple(std::tuple<Ts...> &tuple, std::size_t index, Func function);
}

#include "hades/detail/for_each_tuple.inl"

#endif // !HADES_UTIL_FOR_EACH_TUPLE_HPP