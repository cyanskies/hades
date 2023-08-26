#include "hades/utility.hpp"

#include <algorithm>
#include <cassert>
#include <functional>

namespace hades
{
	//based on logic from
	//here: http://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
	template<std::floating_point Float>
	inline bool float_near_equal(Float a, Float b, int32 ulp) noexcept
	{
        return std::abs(a - b) <= std::numeric_limits<Float>::epsilon() * std::abs(a + b) * float_cast(ulp)
			// unless the result is subnormal
			|| std::abs(a - b) < std::numeric_limits<Float>::min();
	}

	template<std::floating_point Float>
	bool float_rounded_equal(Float a, Float b, detail::round_nearest_t) noexcept
	{
		return std::round(a) == std::round(b);
	}

	template<std::floating_point Float>
	bool float_rounded_equal(Float a, Float b, detail::round_down_t) noexcept
	{
		return std::floor(a) == std::floor(b);
	}

	template<std::floating_point Float>
	bool float_rounded_equal(Float a, Float b, detail::round_up_t) noexcept
	{
		return std::ceil(a) == std::ceil(b);
	}

	template<std::floating_point Float>
	bool float_rounded_equal(Float a, Float b, detail::round_towards_zero_t) noexcept
	{
		return std::trunc(a) == std::trunc(b);
	}

	namespace detail
	{
		template<std::integral Integral, std::floating_point Float, rounding_tag T>
		constexpr Integral integral_do_cast(Float f, T) noexcept
		{
			static_assert(is_round_tag_v<T>);
			if constexpr(std::is_same_v<round_nearest_t, T>)
				return static_cast<Integral>(std::round(f));
			else if constexpr(std::is_same_v<round_down_t, T>)
				return static_cast<Integral>(std::floor(f));
			else if constexpr (std::is_same_v<round_up_t, T>)
				return static_cast<Integral>(std::ceil(f));
			else if constexpr (std::is_same_v<round_towards_zero_t, T>)
				return static_cast<Integral>(f);
		}
	}

	template<std::integral Integral, typename Ty, detail::rounding_tag RoundingTag>
		requires std::is_arithmetic_v<Ty>
	constexpr Integral integral_cast(Ty f, RoundingTag t)
	{
		if constexpr (std::is_integral_v<Ty>)
			return integer_cast<Integral>(f);
		else
		{
			if (f > static_cast<Ty>(std::numeric_limits<Integral>::max()))
				throw overflow_error{ "overflow_error value is larger than target type can hold" };
			else if (f < static_cast<Ty>(std::numeric_limits<Integral>::min()))
				throw overflow_error{ "overflow_error value is smaller than target type can hold" };
			return detail::integral_do_cast<Integral>(f, t);
		}
	}

	template<std::integral Integral, typename Ty, detail::rounding_tag RoundingTag>
		requires std::is_arithmetic_v<Ty>
	constexpr Integral integral_clamp_cast(Ty f, RoundingTag t) noexcept
	{
		if constexpr (std::is_integral_v<Ty>)
			return integer_clamp_cast<Integral>(f);
		else
		{
			if (f > static_cast<Ty>(std::numeric_limits<Integral>::max()))
				return std::numeric_limits<Integral>::max();
			else if (f < static_cast<Ty>(std::numeric_limits<Integral>::min()))
				return std::numeric_limits<Integral>::min();
			return detail::integral_do_cast<Integral>(f, t);
		}
	}

	template<std::floating_point Float, typename T>
		requires std::is_arithmetic_v<T>
	constexpr Float float_cast(T t)
	{
		// handle different float type sizes
		if constexpr (std::is_floating_point_v<T> && sizeof(Float) < sizeof(T))
		{
			if(t > static_cast<T>(std::numeric_limits<Float>::max()))
				throw overflow_error{ "overflow_error value is larger than target type can hold" };
			else if (t < static_cast<T>(std::numeric_limits<Float>::lower()))
				throw overflow_error{ "overflow_error value is smaller than target type can hold" };
		}

        // all intergrals fit in all floats (but may may loose precision)
		return static_cast<Float>(t);
	}

	template<std::floating_point Float, typename T>
		requires std::is_arithmetic_v<T>
	constexpr Float float_clamp_cast(T t) noexcept
	{
		// handle different float type sizes
		if constexpr (std::is_floating_point_v<T> && sizeof(Float) < sizeof(T))
		{
			if (t > static_cast<T>(std::numeric_limits<Float>::max()))
				return std::numeric_limits<Float>::max();
			else if (t < static_cast<T>(std::numeric_limits<Float>::lower()))
				return std::numeric_limits<Float>::min();
		}

		// all intergrals fit in all floats
		return static_cast<Float>(t);
	}

	template<typename T>
	constexpr std::make_unsigned_t<T> unsigned_cast(T value)
	{
		using U = std::make_unsigned_t<T>;

		if constexpr (std::is_unsigned_v<T>)
			return value;
		else if (value >= T{})
			return static_cast<U>(value);
		else
			throw overflow_error{ "overflow_error; tried to cast a negative value to unsigned" };
	}

	template<typename T>
	constexpr std::make_unsigned_t<T> unsigned_clamp_cast(T value) noexcept
	{
		using U = std::make_unsigned_t<T>;

		if constexpr (std::is_unsigned_v<T>)
			return value;
		else if (value > T{})
			return static_cast<U>(value);
		else
			return U{};
	}

	template<typename T>
	constexpr std::make_signed_t<T> signed_cast(T value)
	{
		using S = std::make_signed_t<T>;
		if constexpr (std::is_signed_v<T>)
			return value;
		else if (value > static_cast<T>(std::numeric_limits<S>::max()))
			throw overflow_error{ "overflow_error; value was too large for signed type" };
		else
			return static_cast<S>(value);
	}

	template<typename T>
	constexpr std::make_signed_t<T> signed_clamp_cast(T value) noexcept
	{
		using S = std::make_signed_t<T>;
		if constexpr (std::is_signed_v<T>)
			return value;
		else if (value > static_cast<T>(std::numeric_limits<S>::max()))
			return std::numeric_limits<S>::max();
		else
			return static_cast<S>(value);
	}

	template<typename T>
	constexpr auto sign_swap_cast(T value)
	{
		if constexpr (std::is_signed_v<T>)
			return unsigned_cast(value);
		else
			return signed_cast(value);
	}

	template<typename T>
	constexpr auto sign_swap_clamp_cast(T value) noexcept
	{
		if constexpr (std::is_signed_v<T>)
			return unsigned_clamp_cast(value);
		else
			return signed_clamp_cast(value);
	}

	template<typename T, typename U>
	constexpr T size_cast(U i)
	{
		static_assert(std::is_integral_v<T> && std::is_integral_v<U>,
			"size_cast only supports integers");

		static_assert(std::is_signed_v<T> == std::is_signed_v<U>,
			"size_cast integers must both be signed, or both unsigned");

		if constexpr (sizeof(T) >= sizeof(U))
			return i;
		else
		{
			//only need to check min value if integer types are signed
			if constexpr (std::is_signed_v<T>)
				if (i < static_cast<U>(std::numeric_limits<T>::min()))
					throw overflow_error{ "overflow_error value is smaller than target type can hold" };
			
			//check max value
			if (i > static_cast<U>(std::numeric_limits<T>::max()))
				throw overflow_error{ "overflow_error value is larger than target type can hold" };

			return static_cast<T>(i);
		}
	}
	
	template<typename T, typename U>
	constexpr T size_clamp_cast(U i) noexcept
	{
		static_assert(std::is_integral_v<T> && std::is_integral_v<U>,
			"size_cast only supports integers");

		static_assert(std::is_signed_v<T> == std::is_signed_v<U>,
			"size_cast integers must both be signed, or both unsigned");

		if constexpr (sizeof(T) >= sizeof(U))
			return i;
		else
		{
			//only need to check min value if integer types are signed
			if constexpr (std::is_signed_v<T>)
				if (i < static_cast<U>(std::numeric_limits<T>::min()))
					return std::numeric_limits<T>::min();

			//check max value
			if (i > static_cast<U>(std::numeric_limits<T>::max()))
				return std::numeric_limits<T>::max();

			return static_cast<T>(i);
		}
	}

	template<typename T, typename U>
	constexpr T integer_cast(U i)
	{
		static_assert(std::is_integral_v<T> && std::is_integral_v<U>,
			"integer_cast only supports integers");

		if constexpr (std::is_same_v<T, U>)
			return i;
		else if constexpr (std::is_signed_v<T> == std::is_signed_v<U>)
			return size_cast<T>(i);
		else
			return size_cast<T>(sign_swap_cast(i));
	}

	template<typename T, typename U>
	constexpr T integer_clamp_cast(U i) noexcept
	{
		static_assert(std::is_integral_v<T> && std::is_integral_v<U>,
			"integer_clamp_cast only supports integers");

		if constexpr (std::is_same_v<T, U>)
			return i;
		else if constexpr (std::is_signed_v<T> == std::is_signed_v<U>)
			return size_clamp_cast<T>(i);
		else
			return size_clamp_cast<T>(sign_swap_clamp_cast(i));
	}

	template<typename Index2D, typename T>
	constexpr T to_1d_index(Index2D pos, T array_width)
	{
		return to_1d_index({ integer_cast<T>(pos.x), integer_cast<T>(pos.y) }, array_width);
	}
	
	template<typename T>
	constexpr T to_1d_index(std::pair<T, T> index, T w)
	{
		if constexpr (std::is_signed_v<T>)
		{
			if (index.first < 0 || index.second < 0)
				throw std::invalid_argument{ "cannot caculate 1d index in negative space" };
		}

		return index.second * w + index.first;
	}

	template<typename Index2D, typename T>
	constexpr Index2D to_2d_index(T i, T w)
	{
		assert(w != 0);

		return Index2D{
			i % w,
			i / w
		};
	}

	template<typename T>
	constexpr std::pair<T, T> to_2d_index(T i, T w)
	{
		return to_2d_index<std::pair<T, T>>(i, w);
	}

	template<typename Enum, std::enable_if_t<std::is_enum_v<Enum>, int>>
	constexpr std::underlying_type_t<Enum> enum_type(Enum e) noexcept
	{
		return static_cast<std::underlying_type_t<Enum>>(e);
	}

	template<typename Enum, std::enable_if_t<std::is_enum_v<Enum>, int>>
	constexpr Enum next(Enum e) noexcept
	{
		auto val = enum_type(e);
		return Enum{ ++val };
	}

	template<typename Func>
	decltype(auto) make_finally(Func&& f)
	{
		static_assert(std::is_nothrow_invocable_v<Func>, 
			"make_finally only accepts a callable that accepts no arguments and is noexcept");

		struct make_finally_return {
				~make_finally_return() noexcept {
					std::invoke(f);
					return;
				}
				Func f;
			};
		return make_finally_return{ f };
	}

	template<typename Container, typename Iter, typename Less, typename Equal>
	Iter remove_duplicates(Container& cont, Iter first, Iter last, Less less, Equal equal) noexcept
	{
		std::stable_sort(first, last, less);
		const auto last_unique = std::unique(first, last, equal);
		return cont.erase(last_unique, last);
	}

	template<typename Container, typename Less, typename Equal>
	typename Container::iterator remove_duplicates(Container& cont, Less less, Equal equal) noexcept
	{
		return remove_duplicates(cont, begin(cont), end(cont), less, equal);
	}

	template <typename Cont>
		requires is_container_adapter_v<Cont>
	auto& get_underlying_container(Cont& q) noexcept
	{
		struct hacked_queue : private Cont
		{
			static decltype(hacked_queue::c)& get_container(Cont& q) noexcept
			{
				// subcontianer is specified to be the member c
				return q.*&hacked_queue::c;
			}
		};

		return hacked_queue::get_container(q);
	}

	template <typename Cont>
		requires is_container_adapter_v<Cont>
	const auto& get_underlying_container(const Cont& q) noexcept
	{
		struct hacked_queue : private Cont
		{
			static const decltype(hacked_queue::c)& get_container(const Cont& q) noexcept
			{
				// subcontianer is specified to be the member c
				return q.*&hacked_queue::c;
			}
		};

		return hacked_queue::get_container(q);
	}
}
