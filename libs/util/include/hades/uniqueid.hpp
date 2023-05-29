#ifndef HADES_UTIL_UNIQUEID_HPP
#define HADES_UTIL_UNIQUEID_HPP

#include <atomic>
#include <cassert>
#include <limits>

#include "hades/types.hpp"

namespace hades
{
	template<typename id_type>
	class unique_id_t
	{
	public:
		using type = id_type;

		struct zero_id_t {};
		static constexpr zero_id_t zero_id{};

		struct next_id_t {};
		static constexpr next_id_t new_id{};

		constexpr unique_id_t(zero_id_t = zero_id) noexcept : _value{ std::numeric_limits<type>::min() }
		{}

		constexpr explicit unique_id_t(next_id_t) noexcept : _value(++_count) 
		{
			assert(_count != std::numeric_limits<type>::max());
		}

		auto operator<=>(const unique_id_t&) const = default;

		constexpr type get() const noexcept { return _value; }

		operator bool() const noexcept
		{
			return *this != zero;
		}

		static unique_id_t<id_type> zero;
	private:
		//initialise the static counter with the types smallest value
		static std::atomic<type> _count;
		type _value;
	};

	template<typename id_type>
	std::atomic<id_type> unique_id_t<id_type>::_count = std::numeric_limits<id_type>::min();

	template<typename id_type>
	unique_id_t<id_type> unique_id_t<id_type>::zero = unique_id_t<id_type>{ zero_id };

	//process wide unique id
	using unique_id = unique_id_t<hades::types::uint64>;
	static_assert(sizeof(unique_id) == sizeof(unique_id::type));

	constexpr auto unique_zero = unique_id{ unique_id::zero_id };
	inline unique_id make_unique_id() noexcept
	{
		return unique_id{ unique_id::new_id };
	}
}

template <typename T> 
struct std::hash<hades::unique_id_t<T>>
{
	size_t operator()(const hades::unique_id_t<T>& key) const noexcept
	{
		const auto h = std::hash<T>{};
		return h(key.get());
	}
};

#endif // hades_util_unique_id_hpp
