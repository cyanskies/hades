#include "hades/utility.hpp"

#include <charconv>
#include <sstream>
#include <string_view>

namespace hades
{
	//replace with logic similar to that displayed in the example
	//here: http://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
	inline bool floatEqual(float a, float b)
	{
		return fabs(a - b) <= std::numeric_limits<float>::epsilon() * 4;
	}

	template<typename T>
	T random(T min, T max)
	{
		if (min == max)
			return min;
		else if (min > max)
			return random(max, min);

		std::uniform_int_distribution<T> random(min, max);

		return random(random_generator);
	}

	inline bool random()
	{
		return random(0, 1) != 0;
	}

	template<typename Container>
	decltype(auto) remove_duplicates(Container &cont)
	{
		return remove_duplicates(cont, std::begin(cont), std::end(cont),
			std::less<typename Container::value_type>{}, std::equal_to<typename Container::value_type>{});
	}

	template<typename Container, typename Less, typename Equal>
	decltype(auto) remove_duplicates(Container &cont, Less less, Equal equal)
	{
		return remove_duplicates(cont, std::begin(cont), std::end(cont), less, equal);
	}

	template<typename Container, typename Iter>
	Iter remove_duplicates(Container &cont, Iter first, Iter last)
	{
		return remove_duplicates(first, last, std::less<typename Iter::value_type>{}, std::equal_to<typename Iter::value_type>{});
	}

	template<typename Container, typename Iter, typename Less, typename Equal>
	Iter remove_duplicates(Container &cont, Iter first, Iter last, Less less, Equal equal)
	{
		std::stable_sort(first, last, less);
		const auto last_unique = std::unique(first, last, equal);
		return cont.erase(last_unique, std::end(cont));
	}

	template<typename Out>
	void split(const std::string &s, char delim, Out result)
	{
		std::stringstream ss;
		ss.str(s);
		std::string item;
		while (std::getline(ss, item, delim)) {
			*(result++) = item;
		}
	}

	template <typename Out>
	void split(std::string_view sv, char delim, Out result) 
	{
		while (!sv.empty())
		{
			std::string_view output;
			auto delim_pos = sv.find_first_of(delim);
			if (delim_pos == std::string_view::npos)
			{
				output = sv;
				sv = std::string_view();
			}
			else
			{
				//grab the chunk before the delim
				output = sv.substr(0, delim_pos);
				sv = sv.substr(delim_pos + 1, sv.length() - delim_pos);
			}

			*(result++) = output;
		}
	}

	template<typename T>
	types::string to_string(T value)
	{
		return std::to_string(value);
	}

	template<class First, class Last>
	types::string to_string(First begin, Last end, std::string_view delim)
	{
		if (begin == end) return types::string();

		const auto delimiter = to_string(delim);
		auto out = to_string(*begin);

		while (++begin != end)
			out += delimiter + to_string(*begin);

		return out;
	}

	struct bracket_wrap_t {};

	constexpr bracket_wrap_t bracket_wrap{};

	template<class First, class Last>
	types::string to_string(First begin, Last end, std::string_view delim, bracket_wrap_t wrap)
	{
		const auto str = to_string(begin, end, delim);

		using namespace std::string_literals;
		return "["s + str + "]"s;
	}

	template<class First, class Last>
	types::string to_string(First begin, Last end)
	{
		using namespace std::string_view_literals;
		return to_string(begin, end, ", "sv, bracket_wrap);
	}

	template<typename T>
	T from_string(std::string_view str)
	{
		auto value = T{};
		const auto result = std::from_chars(str.data(), str.data() + str.size(), value);

		//TODO: check result

		return value;
	}

	template<typename T>
	T vector_from_string(std::string_view str)
	{
		//either : T
		// or	 : T, T, T, T, T
		// or	 : [T, T, T, T, T, T]

		using value_type = T::value_type;

		//remove the braces at begining and end if present
		const auto first_brace = str.find_first_of('[');
		if (first_brace != std::string_view::npos)
			str.remove_prefix(first_brace + 1);

		const auto last_brace = str.find_last_of(']');
		if (last_brace != std::string_view::npos)
			str.remove_suffix(str.size() - last_brace);

		//split into csv
		std::vector<std::string_view> elements;
		split(str, ',', std::back_inserter(elements));

		if (std::any_of(std::begin(elements), std::end(elements), [](auto &&s) {
			return s.empty() //return true if the str is empty or consists of spaces
				|| std::all_of(std::begin(s), std::end(s), [](auto &&c) {return c == ' '; });
		}))
		{
			return T{};
		}

		//convert each one into T
		std::vector<value_type> out;
		for (const auto &e : elements)
			out.push_back(from_string<value_type>(e));

		return out;
	}
}