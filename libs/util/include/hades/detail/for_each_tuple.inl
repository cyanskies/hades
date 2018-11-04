#include "hades/for_each_tuple.hpp"

//recursive worker templates for 
//the for each
namespace hades::detail
{
	template<typename Func, std::size_t Count, typename ...Ts>
	inline void for_each_worker(std::tuple<Ts...> &t, Func f)
	{
		if constexpr (Count >= std::tuple_size_v<std::tuple<Ts...>>)
			return;
		else
		{
			using T = std::tuple_element_t<Count, std::tuple<Ts...>>;
			if constexpr (std::is_invocable_v<Func, T, std::size_t>)
				std::invoke(f, std::get<Count>(t), Count);
			else
				std::invoke(f, std::get<Count>(t));

			for_each_worker<Func, Count + 1, Ts...>(t, f);
		}
	}

	template<typename Func, std::size_t Count, typename ...Ts>
	inline void for_index_worker(std::tuple<Ts...> &t, std::size_t index, Func f)
	{
		if constexpr (Count >= std::tuple_size_v<std::tuple<Ts...>>)
			return;
		else
		{
			if (Count == index)
			{
				std::invoke(f, std::get<Count>(t));
				return;
			}

			for_each_worker<Func, Count + 1, Ts...>(t, f);
		}
	}
}

namespace hades
{
	template<typename Func, typename ...Ts>
	void for_each_tuple(std::tuple<Ts...> &t, Func f)
	{
		detail::for_each_worker < Func, std::size_t{ 0 }, Ts... > (t, f);
	}

	template<typename Func, typename ...Ts>
	void for_index_tuple(std::tuple<Ts...> &t, std::size_t i, Func f)
	{
		assert(i <= std::tuple_size_v<std::tuple<Ts...>>);

		detail::for_index_worker < Func, std::size_t{ 0 }, Ts... > (t, i, f);
	}
}