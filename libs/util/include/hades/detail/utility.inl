#include "hades/utility.hpp"

#include <charconv>
//#include <limits>
#include <sstream>

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

	//remove_duplicates: removes all duplicates from the container
	// can remove only a subrange, or use custom comparitors

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

	//pass a back_inserter to result
	template<typename Out>
	void split(const std::string &s, char delim, Out result) {
		std::stringstream ss;
		ss.str(s);
		std::string item;
		while (std::getline(ss, item, delim)) {
			*(result++) = item;
		}
	}

	template <typename Out>
	void split(std::string_view sv, char delim, Out result) {
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
	types::string to_string(First begin, Last end)
	{
		if (begin == end) return types::string();

		auto out = to_string(*begin);

		while (++begin != end)
			out += " " + to_string(*begin);

		return out;
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
		return T{};
	}
}