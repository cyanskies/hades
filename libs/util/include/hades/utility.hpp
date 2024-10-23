#ifndef HADES_UTIL_UTILITY_HPP
#define HADES_UTIL_UTILITY_HPP

#include <array>
#include <queue> // get_underlying_container
#include <stack> // get_underlying_container
#include <stdexcept>
#include <utility>

#include "hades/types.hpp"
#include "hades/tuple.hpp"

// TODO: break this header up into smaller groups

namespace hades {

	template<typename Func, typename Ret, typename T>
	concept unary_operator = std::is_invocable_r_v<Ret, Func, T>;

	template<typename Func, typename Ret, typename ...Args>
	concept invocable_r = requires (Func&& f, Args... args) {
		{ std::invoke(f, args...) } -> std::same_as<Ret>;
	};


	namespace detail
	{
		using std::lerp;
		template<typename T>
		concept linear_interpable_impl = requires(T t, T t2, float f)
		{
			{ lerp(t, t2, f) } noexcept -> std::same_as<T>;
		};
	}

	// concepts for types that can be called on std::lerp or ADL- lerp
	template<typename T>
	concept linear_interpable = detail::linear_interpable_impl<T>;

	template<std::floating_point Float>
	inline bool float_near_equal(Float a, Float b, int32 units_after_decimal = 2) noexcept;

	namespace detail
	{
		struct round_nearest_t {};
		struct round_down_t {}; 
		struct round_up_t {};
		struct round_towards_zero_t {};

		template<typename T>
		constexpr auto is_round_tag_v = std::is_same_v<round_nearest_t, T> ||
			std::is_same_v<round_down_t, T> ||
			std::is_same_v<round_up_t, T> ||
			std::is_same_v<round_towards_zero_t, T>;
		template<typename T>
		concept rounding_tag = is_round_tag_v<T>;
	}

	constexpr detail::round_nearest_t round_nearest_tag;
	constexpr detail::round_down_t round_down_tag;
	constexpr detail::round_up_t round_up_tag;
	constexpr detail::round_towards_zero_t round_towards_zero_tag;

	template<std::floating_point Float>
	inline bool float_rounded_equal(Float a, Float b, detail::round_nearest_t = round_nearest_tag) noexcept;
	template<std::floating_point Float>
	inline bool float_rounded_equal(Float a, Float b, detail::round_down_t) noexcept;
	template<std::floating_point Float>
	inline bool float_rounded_equal(Float a, Float b, detail::round_up_t) noexcept;
	template<std::floating_point Float>
	inline bool float_rounded_equal(Float a, Float b, detail::round_towards_zero_t) noexcept;

	class overflow_error : public std::overflow_error
	{
	public: 
		using std::overflow_error::overflow_error;
	};

	// convert floating point types to integrals
	// throws overflow error
	template<std::integral Integral, typename T, detail::rounding_tag RoundingTag = detail::round_nearest_t>
		requires std::is_arithmetic_v<T>
	constexpr Integral integral_cast(T, RoundingTag = round_nearest_tag);
	template<std::integral Integral, typename T, detail::rounding_tag RoundingTag = detail::round_nearest_t>
		requires std::is_arithmetic_v<T>
	constexpr Integral integral_clamp_cast(T, RoundingTag = round_nearest_tag) noexcept;

	// Convert between floating types, and convert integrals to float 
	// throws overflow error
	template<std::floating_point Float = float, typename T>
		requires std::is_arithmetic_v<T>
	constexpr Float float_cast(T);
	template<std::floating_point Float = float, typename T>
		requires std::is_arithmetic_v<T>
	constexpr Float float_clamp_cast(T) noexcept;

	//converts value to unsigned, throws overflow_error if out of range
	template<typename T>
	constexpr std::make_unsigned_t<T> unsigned_cast(T value);

	//converts value to unsigned, clamps to representable value if out of range
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

	//converts one integer type to another
	//throws overflow_error if the value doesn't fit in the new range
	template<typename T = int, typename U>
	constexpr T integer_cast(U);

	//converts one integer type to another
	// clamps stored value to representable range
	template<typename T = int, typename U>
	constexpr T integer_clamp_cast(U) noexcept;

	template<typename T, tuple Index2D = std::pair<T, T>>
	constexpr T to_1d_index(Index2D pos, T array_width) noexcept(!std::is_signed_v<T>);

	template<typename Index2D, typename T>
	constexpr Index2D to_2d_index(T index, T array_width) noexcept;

	template<typename T>
	constexpr std::pair<T, T> to_2d_index(T index, T array_width) noexcept;

	//convert Enum value to underlying type
	template<typename Enum, std::enable_if_t<std::is_enum_v<Enum>, int> = 0>
	constexpr std::underlying_type_t<Enum> enum_type(Enum e) noexcept;

	template<typename Enum, std::enable_if_t<std::is_enum_v<Enum>, int> = 0>
	constexpr Enum next(Enum e) noexcept;

	// returns an object that calls f when it goes out of scope
	template<typename Func>
	decltype(auto) make_finally(Func&& f);

	// overloaded func for merging lambdas into functors with overloads
	// See example on: https://en.cppreference.com/w/cpp/utility/variant/visit
	template<class... Ts>
	struct overloaded : Ts... { using Ts::operator()...; };

	//remove_duplicates: removes all duplicates from the container
	// can remove only a subrange, or use custom comparitors
	template<typename Container, typename Iter, typename Less = std::less<>, typename Equal = std::equal_to<>>
	Iter remove_duplicates(Container& cont, Iter first, Iter last, Less less = std::less{}, Equal equal = std::equal_to{}) noexcept;

	template<typename Container, typename Less = std::less<>, typename Equal = std::equal_to<>>
	typename Container::iterator remove_duplicates(Container& cont, Less less = std::less{}, Equal equal = std::equal_to{}) noexcept;

	// Type tests for get_underlying_container
	template<typename T>
	struct is_container_adapter : std::bool_constant<false>
	{};

	template<typename ...Args>
	struct is_container_adapter<std::stack<Args...>> : std::bool_constant<true>
	{};

	template<typename ...Args>
	struct is_container_adapter<std::queue<Args...>> : std::bool_constant<true>
	{};

	template<typename ...Args>
	struct is_container_adapter<std::priority_queue<Args...>> : std::bool_constant<true>
	{};

	template<typename T>
	constexpr auto is_container_adapter_v = is_container_adapter<T>::value;

	// Extracts a reference to the underlying container for a std container adapter
	//			eg. stack, queue, or priority_queue
	// This allows you to inspect the raw data or clear the container more efficiently
	// NOTE: don't mess with the elements of a priority_queue, you'll muck up the 
	//		heap invariants
	template <typename Cont>
		requires is_container_adapter_v<Cont>
	auto& get_underlying_container(Cont& q) noexcept;

	template <typename Cont>
		requires is_container_adapter_v<Cont>
	const auto& get_underlying_container(const Cont& q) noexcept;

	// TODO: vv move this to some function related header vv

	// defines func as an alias for target_function
	// allows for using the std::function-like syntax
	// when needing to specify args and return value
	// Usage: constexpr auto &func = func_ref(target_function);
	// eg. manually specify the return and arguments
	// constexpr auto& n = func_ref<string(int)>(to_string);
	// eg. auto deduce function type
	// constexpr auto& g = func_ref(vector_from_string<std::vector<int>>);
	template<typename Func, std::enable_if_t<std::is_function_v<Func>, int> = 0>
	consteval const Func &func_ref(const Func &func)
	{
		return func;
	}
}

#include "hades/detail/utility.inl"

#endif // !HADES_UTIL_UTILITY_HPP
