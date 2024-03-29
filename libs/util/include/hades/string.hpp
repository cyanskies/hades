#ifndef HADES_UTIL_STRING_HPP
#define HADES_UTIL_STRING_HPP

#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <unordered_map>

//#include "hades/exceptions.hpp"

namespace hades
{
	using string = std::string;

	namespace types
	{
		using hades::string;
	}

	// identify string types, these can all be converted to hades::string
	template<typename T> struct is_string : std::is_convertible<T, std::string_view> {};
	template<typename T>
	constexpr bool is_string_v = is_string<T>::value;

	template<typename T>
	concept string_type = std::is_convertible_v<T, std::string_view>;
	// These are the types that we consider 'string' types
	//	static_assert(is_string_v<std::string>);
	//	static_assert(is_string_v<const char*>);
	//	static_assert(is_string_v<std::string_view>);
	//
	// basically anything that can be converted to string_view
	
	// everything not considered a string_type
	template<typename T>
	concept not_string_type = !string_type<T>;

	// concept for to_string functions
	// TODO: rename string_conversion?
	template<typename Func, typename T>
	concept string_transform = requires (Func&& f, T&& ty) {
		{ std::invoke(f, ty) }->string_type;
	};

	// string_transform accepts nullptr as a valid transform function
	// this is so users can opt in to the to_string functions and templates

	// This produces a functor that calls the appropriate to_string function
	template<typename T, string_transform<T> Func>
	auto make_to_string_functor(Func&& f)
	{
		static_assert(!std::is_same_v<Func, nullptr_t> || requires(T && ty) { { to_string(ty) } -> string_type; },
			"This type needs a custom conversion function, or a new to_string overload needs to be defined for it.");
		if constexpr (string_type<T> && std::same_as<Func, nullptr_t>)
			return std::identity; // strings returned unchannged
		else if constexpr (not_string_type<T> && std::same_as<Func, nullptr_t>)
			return [](const T& ty) { return to_string(ty); }; // try default conversion func
		else // use provided conversion func
			return [func = std::forward<Func>(f)](const T& ty) { return std::invoke(func, ty); };
	}

	//pass a back_inserter to result
	template<typename Out>
	void split(const std::string &s, char delim, Out result);

	template <typename Out>
	void split(std::string_view sv, char delim, Out result);

	constexpr std::string_view trim(std::string_view in) noexcept;

	// catch overloads that can be satisfied by std::to_chars
	template<not_string_type T>
		requires std::integral<T> || std::floating_point<T>
	string to_string(T value);

	// convert string types to hades::string
	template<string_type T>
	string to_string(T&& value)
	{
		return string{ std::forward<T>(value) };
	}

	template<class First, class Last>
	string to_string(First begin, Last end);

	// types that can be converted to string using ADL to_string
	template<typename T>
	concept stringable = not_string_type<T> &&
		requires(T t) { { to_string(t) }->string_type; };

	//thrown by all the from_string functions
	class bad_conversion : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	template<typename T>
	T from_string(std::string_view str);

	template<>
	bool from_string<bool>(std::string_view str);

	template<>
	string from_string<string>(std::string_view str);

	template<>
	std::string_view from_string<std::string_view>(std::string_view str);

	template<typename T, typename FromString>
	T vector_from_string(std::string_view str, FromString&&);

	template<typename T>
	T vector_from_string(std::string_view str);

	template<typename Ty>
		requires std::is_trivially_copyable_v<Ty>
	std::string base64_encode(std::span<Ty> data);

	template<typename Ty>
		requires std::is_default_constructible_v<Ty> && std::is_trivially_copyable_v<Ty>
	std::vector<Ty> base64_decode(std::string_view source);

	// map containers that can be searched with char* and string_view
	template<typename T>
	using map_string = std::map<string, T, std::less<>>;
	using set_string = std::set<string, std::less<>>;

	// hash for unordered containers
	// compares all string types
	struct string_hash 
	{
		using is_transparent = std::true_type;

		constexpr std::size_t operator()(const string_type auto& t)const {
			return std::hash<std::string_view>{}(t);
		}
	};

	// unordered collections that can be searched with char* and string_view
	template<typename T>
	using unordered_map_string = std::unordered_map<string, T, string_hash, std::equal_to<>>;
	using unordered_set_string = std::unordered_map<string, string_hash, std::equal_to<>>;
}

#include "hades/detail/string.inl"

#endif // !HADES_UTIL_STRING_HPP
