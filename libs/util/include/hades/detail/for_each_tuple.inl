#include "hades/for_each_tuple.hpp"

//recursive worker templates for 
//the for each
namespace hades::detail
{
	template<std::size_t Count, typename Tuple, typename Func, typename ...Args>
	inline void for_each_worker(Tuple &t, Func f, Args... args)
	{
		using T = std::tuple_element_t<Count, Tuple>;
		
		if constexpr (std::is_invocable_v<Func, T, std::size_t, Args...>)
			std::invoke(f, std::get<Count>(t), Count, args...);
		else
			std::invoke(f, std::get<Count>(t), args...);

		if constexpr (Count + 1 < std::tuple_size_v<Tuple>)
			for_each_worker<Count + 1>(t, f, args...);
	}

	template<std::size_t Count, typename Tuple, typename Func, typename ...Args>
	inline auto for_each_worker_r(Tuple &t, Func f, Args... args)
	{
		using T = std::tuple_element_t<Count, Tuple>;
		using ResultType = std::invoke_result_t<Func, T, Args...>;

		ResultType result{};

		if constexpr (std::is_invocable_v<Func, T, std::size_t, Args...>)
			result = std::invoke(f, std::get<Count>(t), Count, args...);
		else
			result = std::invoke(f, std::get<Count>(t), args...);

		std::vector<ResultType> result_collection{};
		result_collection.push_back(result);

		if constexpr (Count + 1 < std::tuple_size_v<Tuple>)
		{
			auto new_result = for_each_worker_r<Count + 1>(t, f, args...);
			std::copy(std::begin(new_result), std::end(new_result),
				std::back_inserter(result_collection));
		}

		return result_collection;
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
	template<typename Tuple, typename Func, typename ...Args>
	void for_each_tuple(Tuple &t, Func f, Args... args)
	{
		assert(std::tuple_size_v<Tuple> > 0);
		detail::for_each_worker < std::size_t{ 0 } > (t, f, args...);
	}

	template<typename Tuple, typename Func, typename ...Args>
	auto for_each_tuple_r(Tuple &t, Func f, Args ...args)
	{
		assert(std::tuple_size_v<Tuple> > 0);
		auto result = detail::for_each_worker_r < std::size_t{ 0 } > (t, f, args...);

		std::reverse(std::begin(result), std::end(result));
		return result;
	}

	template<typename Func, typename ...Ts>
	void for_index_tuple(std::tuple<Ts...> &t, std::size_t i, Func f)
	{
		assert(i <= std::tuple_size_v<std::tuple<Ts...>>);

		detail::for_index_worker<Func, std::size_t{0u}>(t, i, f);
	}
}