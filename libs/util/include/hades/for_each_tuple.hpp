#ifndef HADES_UTIL_FOR_EACH_TUPLE_HPP
#define HADES_UTIL_FOR_EACH_TUPLE_HPP

#include <tuple>
#include <vector>

namespace hades
{
	template<typename Tuple, typename Func, typename ...Args>
	void for_each_tuple(Tuple &tuple, Func function, Args... args);

	//returns a vector of the output from each tuple member
	template<typename Tuple, typename Func, typename...Args>
	auto for_each_tuple_r(Tuple &tuple, Func function, Args... args);

	template<typename Func, typename ...Ts>
	void for_index_tuple(std::tuple<Ts...> &tuple, std::size_t index, Func function);
}

#include "hades/detail/for_each_tuple.inl"

#endif // !HADES_UTIL_FOR_EACH_TUPLE_HPP