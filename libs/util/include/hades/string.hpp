#ifndef HADES_UTIL_STRING_HPP
#define HADES_UTIL_STRING_HPP

#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

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
	concept string_type = is_string<T>::value;
	// These are the types that we consider 'string' types
	//	static_assert(is_string_v<std::string>);
	//	static_assert(is_string_v<const char*>);
	//	static_assert(is_string_v<std::string_view>);
	//
	// basically anything that can be converted to string_view

	//pass a back_inserter to result
	template<typename Out>
	void split(const std::string &s, char delim, Out result);

	template <typename Out>
	void split(std::string_view sv, char delim, Out result);

	constexpr std::string_view trim(std::string_view in) noexcept;

	// catch overloads that can be satisfied by std::to_chars
	template<typename T, std::enable_if_t<!is_string_v<T>, int> = 0>
	string to_string(T value);

	// convert string types to hades::string
	template<typename T, std::enable_if_t<is_string_v<T>, int> = 0>
	string to_string(T&& value)
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
	std::string_view from_string<std::string_view>(std::string_view str) noexcept;

	template<typename T, typename FromString>
	T vector_from_string(std::string_view str, FromString&&);

	template<typename T>
	T vector_from_string(std::string_view str);
}

#include "hades/detail/string.inl"

#endif // !HADES_UTIL_STRING_HPP
