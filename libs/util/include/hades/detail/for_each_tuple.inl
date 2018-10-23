#include "hades/for_each_tuple.hpp"

//recursive worker templates for 
//the for each
namespace hades::detail
{
	template<typename Func, std::size_t Count, typename ...Ts>
	constexpr inline std::enable_if_t<std::tuple_size_v<std::tuple<Ts...>> <= Count>
		for_each_worker(const std::tuple<Ts...>&, Func)
	{}

	template<typename Func, std::size_t Count, typename ...Ts>
	constexpr inline std::enable_if_t < Count < std::tuple_size_v<std::tuple<Ts...>>>
		for_each_worker(const std::tuple<Ts...> &t, Func f)
	{
		std::invoke(f, std::get<Count>(t));
		for_each_worker<Func, Count + 1, Ts...>(t, f);
	}

	template<typename Func, std::size_t Count, typename ...Ts>
	inline std::enable_if_t<std::tuple_size_v<std::tuple<Ts...>> <= Count> for_each_worker(std::tuple<Ts...>&, Func)
	{}

	template<typename Func, std::size_t Count, typename ...Ts>
	inline std::enable_if_t<Count < std::tuple_size_v<std::tuple<Ts...>>> for_each_worker(std::tuple<Ts...> &t, Func f)
	{
		std::invoke(f, std::get<Count>(t));
		for_each_worker<Func, Count + 1, Ts...>(t, f);
	}
}

namespace hades
{
	template<typename Func, typename ...Ts>
	constexpr inline void for_each_tuple(const std::tuple<Ts...> &t, Func f)
	{
		detail::for_each_worker < Func, std::size_t{ 0 }, Ts... > (t, f);
	}

	template<typename Func, typename ...Ts>
	void for_each_tuple(std::tuple<Ts...> &t, Func f)
	{
		detail::for_each_worker < Func, std::size_t{ 0 }, Ts... > (t, f);
	}
}