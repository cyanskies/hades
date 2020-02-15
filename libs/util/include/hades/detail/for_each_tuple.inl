#include "hades/for_each_tuple.hpp"

#include <tuple>

//recursive worker templates for 
//the for each
namespace hades::detail
{
	template<std::size_t Count, typename Func, typename Tuple, typename ...Args>
	constexpr bool nothrow_invokable_v() noexcept
	{
		using T = std::tuple_element_t<Count, Tuple>;

		if constexpr (std::is_invocable_v<Func, T, std::size_t, Args...>)
			return std::is_nothrow_invocable_v<Func, T, std::size_t, Args...>;
		else
			return std::is_nothrow_invocable_v<Func, T, Args...>;
	}

	template<std::size_t Count, typename Tuple, typename Func, typename ...Args>
	inline void for_each_worker(Tuple &t, Func f, Args... args)
		noexcept(nothrow_invokable_v<Count, Func, Tuple, Args...>())
	{
		using T = std::tuple_element_t<Count, Tuple>;

		if constexpr (std::is_invocable_v<Func, T, std::size_t, Args...>)
			std::invoke(f, std::get<Count>(t), Count, args...);
		else
			std::invoke(f, std::get<Count>(t), args...);

		if constexpr (Count + 1 < std::tuple_size_v<Tuple>)
			for_each_worker<Count + 1>(t, f, args...);
	}

	template<std::size_t Count, typename Func, typename Tuple, typename ...Args>
	constexpr bool nothrow_invokable_r_v() noexcept
	{
		using T = std::tuple_element_t<Count, Tuple>;

		if constexpr (std::is_invocable_v<Func, T, std::size_t, Args...>)
		{
			using Result = std::invoke_result_t<Func, T, std::size_t, Args...>;
			return std::is_nothrow_invocable_r_v<Result, Func, T, std::size_t, Args...>;
		}
		else
		{
			using Result = std::invoke_result_t<Func, T, Args...>;
			return std::is_nothrow_invocable_v<Result, Func, T, Args...>;
		}
	}

	template<std::size_t Count, typename Tuple, typename Func, typename ...Args>
    inline decltype(auto) for_each_worker_r(Tuple &t, Func f, Args... args)
		noexcept(nothrow_invokable_r_v<Count, Func, Tuple, Args...>())
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
			result_collection.insert(std::end(result_collection), std::begin(new_result), std::end(new_result));
		}

		return result_collection;
	}

	template<std::size_t Count, typename Func, typename Tuple, typename ...Args>
	inline void for_index_worker(Tuple &t, std::size_t i, Func f, Args... args) 
		noexcept(std::is_nothrow_invocable_v<Func, std::tuple_element_t<Count, Tuple>, Args...>)
	{
		if (Count == i)
			std::invoke(f, std::get<Count>(t));
		else if constexpr(Count + 1 < std::tuple_size_v<Tuple>)
			for_index_worker<Count + 1>(t, i, f, args...);
	}
}

namespace hades
{
	//TODO: rewrite these to not be recursive

	template<typename Tuple, typename Func, typename ...Args>
	void for_each_tuple(Tuple &t, Func f, Args... args) 
		noexcept(noexcept(detail::for_each_worker<std::size_t{ 0u }>(t, f, args...)))
	{
		assert(std::tuple_size_v<Tuple> > 0);
		detail::for_each_worker < std::size_t{ 0 } > (t, f, args...);
	}

	template<typename Tuple, typename Func, typename ...Args>
    decltype(auto) for_each_tuple_r(Tuple &t, Func f, Args ...args)
		noexcept(noexcept(detail::for_each_worker_r < std::size_t{ 0u }>(t, f, args...)))
	{
		assert(std::tuple_size_v<Tuple> > 0);
		auto result = detail::for_each_worker_r < std::size_t{ 0u } > (t, f, args...);

		std::reverse(std::begin(result), std::end(result));
		return result;
	}

	template<typename Func, typename Tuple, typename ...Args>
	void for_index_tuple(Tuple &t, std::size_t i, Func f, Args... args) 
		noexcept(noexcept(detail::for_index_worker < std::size_t{ 0u }>(t, i ,f, args...)))
	{
		assert(i < std::tuple_size_v<Tuple>);

		detail::for_index_worker<std::size_t{0u}>(t, i, f, args...);
	}
}
