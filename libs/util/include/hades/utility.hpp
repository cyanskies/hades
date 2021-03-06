#ifndef HADES_UTIL_UTILITY_HPP
#define HADES_UTIL_UTILITY_HPP

#include <array>
#include <random>
#include <stdexcept>
#include <utility>

#include "hades/types.hpp"

namespace hades {
	namespace
	{
		std::random_device rd{};
		std::default_random_engine random_generator{ rd() };
	}

	template<typename T>
	struct lerpable : public std::bool_constant< std::is_floating_point_v<T>> {};

	template<typename T>
	constexpr auto lerpable_v = lerpable<T>::value;

	template<typename Float,
		typename std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
		constexpr Float lerp(Float a, Float b, Float t) noexcept;

	//NOTE: define provided to allow compilation of path that will never be called
	template<typename T,
		typename std::enable_if_t<!lerpable_v<T>, int> = 0>
		T lerp(T, T, float)
	{
		throw std::logic_error{ "called lerp with a non-arithmetic type" };
	}

	//replace with logic similar to that displayed in the example
	//here: http://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
	template<typename Float, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	inline bool float_near_equal(Float a, Float b, int32 units_after_decimal = 2) noexcept;

	namespace detail
	{
		struct round_nearest_t {};
		struct round_down_t {}; 
		struct round_up_t {};
		struct round_towards_zero_t {};
	}

	constexpr detail::round_nearest_t round_nearest_tag;
	constexpr detail::round_down_t round_down_tag;
	constexpr detail::round_up_t round_up_tag;
	constexpr detail::round_towards_zero_t round_towards_zero_tag;

	template<typename Float, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	inline bool float_rounded_equal(Float a, Float b, detail::round_nearest_t = round_nearest_tag) noexcept;
	template<typename Float, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	inline bool float_rounded_equal(Float a, Float b, detail::round_down_t) noexcept;
	template<typename Float, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	inline bool float_rounded_equal(Float a, Float b, detail::round_up_t) noexcept;
	template<typename Float, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	inline bool float_rounded_equal(Float a, Float b, detail::round_towards_zero_t) noexcept;

	class overflow_error : public std::overflow_error
	{
	public: 
		using std::overflow_error::overflow_error;
	};

	//converts value to unsigned, throws overflow_error if out of range
	template<typename T>
	constexpr std::make_unsigned_t<T> unsigned_cast(T value);

	//converts value to unsigned clamps to representable value if out of range
	template<typename T>
	constexpr std::make_unsigned_t<T> unsigned_clamp_cast(T value) noexcept;

	//converts value to signed, throws overflow_error if out of range
	template<typename T>
	constexpr std::make_signed_t<T> signed_cast(T value);

	//converts value to signed, clamps to representable values
	template<typename T>
	constexpr std::make_signed_t<T> signed_clamp_cast(T value) noexcept;

	// casts T to its signed or unsigned type
	// throws overflow_error if the stored value becomes unrepresentable
	template<typename T>
	constexpr auto sign_swap_cast(T value);

	// casts T to its signed or unsigned type
	// clamps value to representable range
	template<typename T>
	constexpr auto sign_swap_clamp_cast(T value) noexcept;

	template<typename T, typename U>
	constexpr T size_cast(U);

	template<typename T, typename U>
	constexpr T size_clamp_cast(U) noexcept;

	//converts one integer type to another
	//throws overflow_error if the value doesn't fit in the new range
	template<typename T, typename U>
	constexpr T integer_cast(U);

	//converts one integer type to another
	// clamps stored value to representable range
	template<typename T, typename U>
	constexpr T integer_clamp_cast(U) noexcept;

	template<typename T>
	T random(T min, T max);

	inline bool random();

	template<typename Iter>
	typename std::iterator_traits<Iter>::reference
		random_element(Iter first, Iter last);

	template<typename Index2D, typename T>
	T to_1d_index(Index2D pos, T array_width);

	template<typename T>
	T to_1d_index(std::pair<T, T> index, T array_width);

	template<typename Index2D, typename T>
	Index2D to_2d_index(T index, T array_width);

	template<typename T>
	std::pair<T, T> to_2d_index(T index, T array_width);

	//convert Enum value to underlying type
	template<typename Enum, std::enable_if_t<std::is_enum_v<Enum>, int> = 0>
	constexpr std::underlying_type_t<Enum> enum_type(Enum e) noexcept;

	template<typename Enum, std::enable_if_t<std::is_enum_v<Enum>, int> = 0>
	constexpr Enum next(Enum e) noexcept;

	template<typename Func>
	struct finally_t
	{
		~finally_t() noexcept
		{
			std::invoke(f);
		}

	private:
		Func f;
	};

	// returns an object that calls f when it goes out of scope
	template<typename Func>
	finally_t<Func> make_finally(Func&& f);

	//remove_duplicates: removes all duplicates from the container
	// can remove only a subrange, or use custom comparitors

	template<typename Container>
	decltype(auto) remove_duplicates(Container &cont);

	template<typename Container, typename Less>
	decltype(auto) remove_duplicates(Container &cont, Less less);

	template<typename Container, typename Less, typename Equal>
	decltype(auto) remove_duplicates(Container &cont, Less less, Equal equal);

	template<typename Container, typename Iter>
	Iter remove_duplicates(Container &cont, Iter first, Iter last);

	template<typename Container, typename Iter, typename Less, typename Equal>
	Iter remove_duplicates(Container &cont, Iter first, Iter last, Less less, Equal equal);

	//TODO: split all of these into a string header

	//pass a back_inserter to result
	template<typename Out>
	void split(const std::string &s, char delim, Out result);

	template <typename Out>
	void split(std::string_view sv, char delim, Out result);

	std::string_view trim(std::string_view);

	// catch overloads that can be satisfied by std::to_string
	template<typename T,
		std::enable_if_t< !is_string_v<T> &&
		std::is_void_v<std::void_t<decltype(std::to_string(std::declval<T>))>>
		, int> = 0>
	string to_string(T value) { return std::to_string(value); }

	// convert string types to hades::string
	template<typename T, std::enable_if_t<is_string_v<T>, int> = 0>
	string to_string(T&& value) noexcept
	{
		return string{ std::forward<T>(value) };
	}

	template<class First, class Last>
	string to_string(First begin, Last end);

	//thrown by all the from_string functions
	class bad_conversion : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	template<typename T>
	T from_string(std::string_view str);

	template<>
	bool from_string<bool>(std::string_view str) noexcept;

	template<>
	string from_string<string>(std::string_view str);

	template<>
	inline std::string_view from_string<std::string_view>(std::string_view str) noexcept
	{ return str; }

	template<typename T, typename FromString>
	T vector_from_string(std::string_view str, FromString);

	template<typename T>
	T vector_from_string(std::string_view str);
}

#include "hades/detail/utility.inl"

#endif // !HADES_UTIL_UTILITY_HPP
