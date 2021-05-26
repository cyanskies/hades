#include "hades/utility.hpp"

#include <functional>

namespace hades
{
	template<typename Float,
		typename std::enable_if_t<std::is_floating_point_v<Float>, int>>
		constexpr Float lerp(Float a, Float b, Float t) noexcept
	{
		//algorithm recommended for consistancy in P0811R2 : https://wg21.link/p0811r2
		if (a <= 0 && b >= 0 ||
			a >= 0 && b <= 0)
			return t * b + (1.f - t) * a;

		if (float_near_equal(t, 1.f)) return b;

		const auto x = a + t * (b - a);
		return t > 1 == b > a ? std::max(b, x) : std::min(b, x);
	}

	//based on logic from
	//here: http://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
	template<typename Float, std::enable_if_t<std::is_floating_point_v<Float>, int>>
	inline bool float_near_equal(Float a, Float b, int32 ulp) noexcept
	{
		return std::abs(a - b) <= std::numeric_limits<Float>::epsilon() * std::abs(a + b) * ulp
			// unless the result is subnormal
			|| std::abs(a - b) < std::numeric_limits<Float>::min();
	}

	template<typename Float, std::enable_if_t<std::is_floating_point_v<Float>, int>>
	bool float_rounded_equal(Float a, Float b, detail::round_nearest_t) noexcept
	{
		return std::round(a) == std::round(b);
	}

	template<typename Float, std::enable_if_t<std::is_floating_point_v<Float>, int>>
	bool float_rounded_equal(Float a, Float b, detail::round_down_t) noexcept
	{
		return std::floor(a) == std::floor(b);
	}

	template<typename Float, std::enable_if_t<std::is_floating_point_v<Float>, int>>
	bool float_rounded_equal(Float a, Float b, detail::round_up_t) noexcept
	{
		return std::ceil(a) == std::ceil(b);
	}

	template<typename Float, std::enable_if_t<std::is_floating_point_v<Float>, int>>
	bool float_rounded_equal(Float a, Float b, detail::round_towards_zero_t) noexcept
	{
		return std::trunc(a) == std::trunc(b);
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

	namespace detail
	{
		template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
		T random(T min, T max)
		{
			std::uniform_int_distribution<T> random{ min, max };
			return random(random_generator);
		}

		template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
		T random(T min, T max)
		{
			std::uniform_real_distribution<T> random{ min, max };
			return random(random_generator);
		}
	}

	template<typename T>
	T random(T min, T max)
	{
		if (min == max)
			return min;
		else if (min > max)
			return detail::random(max, min);

		return detail::random(min, max);
	}

	template<typename Iter>
	typename std::iterator_traits<Iter>::reference random_element(Iter first, Iter last)
	{
		if (first == last)
			return *first;

		const auto dist = std::distance(first, last) - 1;
		const auto target = random(decltype(dist){}, dist);

		std::advance(first, target);
		return *first;
	}

	template<typename Index2D, typename T>
	T to_1d_index(Index2D pos, T array_width)
	{
		return to_1d_index({ integer_cast<T>(pos.x), integer_cast<T>(pos.y) }, array_width);
	}
	
	template<typename T>
	T to_1d_index(std::pair<T, T> index, T w)
	{
		if constexpr (std::is_signed_v<T>)
		{
			if (index.first < 0 || index.second < 0)
				throw std::invalid_argument{ "cannot caculate 1d index in negative space" };
		}

		return index.second * w + index.first;
	}

	template<typename Index2D, typename T>
	Index2D to_2d_index(T i, T w)
	{
		assert(w != 0);

		return Index2D{
			i % w,
			i / w
		};
	}

	template<typename T>
	std::pair<T, T> to_2d_index(T i, T w)
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
		return Enum{ enum_type(e) + 1 };
	}

	inline bool random()
	{
		return random(0, 1) != 0;
	}

	template<typename Func>
	finally_t<Func> make_finally(Func&& f)
	{
		static_assert(std::is_nothrow_invocable_v<Func>, 
			"make_finally only accepts a callable that accepts no arguments and is noexcept");
		return finally_t<Func>{std::forward<Func>(f)};
	}

	template<typename Container>
	decltype(auto) remove_duplicates(Container &cont)
	{
		return remove_duplicates(cont,
			std::less<typename Container::value_type>{});
	}

	template<typename Container, typename Less>
	decltype(auto) remove_duplicates(Container &cont, Less less)
	{
		return remove_duplicates(cont, less,
			std::equal_to<typename Container::value_type>{});
	}

	template<typename Container, typename Less, typename Equal>
	decltype(auto) remove_duplicates(Container &cont, Less less, Equal equal)
	{
		return remove_duplicates(cont, std::begin(cont), std::end(cont), less, equal);
	}

	template<typename Container, typename Iter>
	Iter remove_duplicates(Container &cont, Iter first, Iter last)
	{
		return remove_duplicates(cont, first, last, std::less<std::iterator_traits<Iter>::value_type>{}, std::equal_to<std::iterator_traits<Iter>::value_type>{});
	}

	template<typename Container, typename Iter, typename Less, typename Equal>
	Iter remove_duplicates(Container &cont, Iter first, Iter last, Less less, Equal equal)
	{
		std::stable_sort(first, last, less);
		const auto last_unique = std::unique(first, last, equal);
		return cont.erase(last_unique, last);
	}
}
