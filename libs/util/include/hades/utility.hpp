#ifndef HADES_UTIL_UTILITY_HPP
#define HADES_UTIL_UTILITY_HPP

#include <random>

#include "hades/types.hpp"

namespace hades {
	namespace
	{
		std::random_device rd{};
		std::default_random_engine random_generator{ rd() };
	}

	//replace with logic similar to that displayed in the example
	//here: http://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
	inline bool floatEqual(float a, float b);

	template<typename T>
	T random(T min, T max);

	inline bool random();

	//remove_duplicates: removes all duplicates from the container
	// can remove only a subrange, or use custom comparitors

	template<typename Container>
	decltype(auto) remove_duplicates(Container &cont);

	template<typename Container, typename Less, typename Equal>
	decltype(auto) remove_duplicates(Container &cont, Less less, Equal equal);

	template<typename Container, typename Iter>
	Iter remove_duplicates(Container &cont, Iter first, Iter last);

	template<typename Container, typename Iter, typename Less, typename Equal>
	Iter remove_duplicates(Container &cont, Iter first, Iter last, Less less, Equal equal);

	//pass a back_inserter to result
	template<typename Out>
	void split(const std::string &s, char delim, Out result);

	template <typename Out>
	void split(std::string_view sv, char delim, Out result);

	std::string_view trim(std::string_view);

	template<typename T>
	types::string to_string(T value);

	template<>
	types::string to_string<const char*>(const char* value);
	template<>
	types::string to_string<types::string>(types::string value);
	template<>
	types::string to_string<std::string_view>(std::string_view value);

	template<class First, class Last>
	types::string to_string(First begin, Last end);

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

	template<typename T, typename FromString, typename = std::enable_if_t<std::is_invocable_r_v<T, FromString, std::string_view>>>
	T vector_from_string(std::string_view str, FromString);

	template<typename T>
	T vector_from_string(std::string_view str);
}

#include "hades/detail/utility.inl"

#endif // !HADES_UTIL_UTILITY_HPP