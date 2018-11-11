#include "hades/for_each_tuple.hpp"

//recursive worker templates for 
//the for each
namespace hades::detail
{
	template<typename Func, std::size_t Count, typename Tuple>
	inline void for_each_worker(Tuple &t, Func f)
	{
		using T = std::tuple_element_t<Count, Tuple>;
		if constexpr (std::is_invocable_v<Func, T, std::size_t>)
			std::invoke(f, std::get<Count>(t), Count);
		else
			std::invoke(f, std::get<Count>(t));

		if constexpr(Count + 1 < std::tuple_size_v<Tuple>)
			for_each_worker<Func, Count + 1, Tuple>(t, f);
	}

	template<typename Func, std::size_t Count, typename Tuple>
	inline void for_index_worker(Tuple &t, std::size_t i, Func f)
	{
		if (Count == i)
			std::invoke(f, std::get<Count>(t));
		else if constexpr(Count + 1 < std::tuple_size_v<Tuple>)
			for_index_worker<Func, Count + 1, Tuple>(t, i, f);
	}
}

namespace hades
{
	template<typename Func, typename ...Ts>
	void for_each_tuple(const std::tuple<Ts...> &t, Func f)
	{
		assert(std::tuple_size_v<std::tuple<Ts...>> > 0);
		detail::for_each_worker<Func, std::size_t{0u}>(t, f);
	}


	template<typename Func, typename ...Ts>
	void for_each_tuple(std::tuple<Ts...> &t, Func f)
	{
		assert(std::tuple_size_v<std::tuple<Ts...>> > 0);
		detail::for_each_worker<Func, std::size_t{0u}>(t, f);
	}

	template<typename Func, typename ...Ts>
	void for_index_tuple(std::tuple<Ts...> &t, std::size_t i, Func f)
	{
		assert(i <= std::tuple_size_v<std::tuple<Ts...>>);


		detail::for_index_worker<Func, std::size_t{0u}>(t, i, f);
	}
}