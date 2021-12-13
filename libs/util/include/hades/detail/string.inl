#include "hades/string.hpp"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <vector>

namespace hades
{
	template<typename Out>
	void split(const std::string& s, char delim, Out result)
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
				sv = std::string_view{};
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

	constexpr std::string_view trim(std::string_view in) noexcept
	{
		if (empty(in))
			return {};
		const auto start = in.find_first_not_of(' ');
		const auto end = in.find_last_not_of(' ');

		return in.substr(start, end - start + 1);
	}

	template<class First, class Last>
	string to_string(First begin, Last end, std::string_view delim)
	{
		if (begin == end) return string{};

		const auto delimiter = to_string(delim);
		auto out = to_string(*begin);

		while (++begin != end)
			out += delimiter + to_string(*begin);

		return out;
	}

	struct bracket_wrap_t {};

	constexpr bracket_wrap_t bracket_wrap{};

	template<class First, class Last>
	string to_string(First begin, Last end, std::string_view delim, bracket_wrap_t)
	{
		const auto str = to_string(begin, end, delim);

		using namespace std::string_literals;
		return "["s + str + "]"s;
	}

	template<class First, class Last>
	string to_string(First begin, Last end)
	{
		using namespace std::string_view_literals;
		return to_string(begin, end, ", "sv, bracket_wrap);
	}

	template<typename T>
	T from_string(std::string_view str)
	{
		//remove spaces
		const auto s = trim(str);

		if (s.empty())
			return {};

		auto value = T{};
		const auto result = std::from_chars(s.data(), s.data() + s.size(), value);

		switch (result.ec)
		{
		case std::errc::invalid_argument: 
			throw bad_conversion{ "from_string: string cannot be converted" };
		case std::errc::result_out_of_range:
			throw bad_conversion{ "from_string: result out of range" };
		default:
			return value;
		}
	}

	namespace detail
	{
		template<typename T, typename FromString>
		T vector_from_string_impl(std::string_view str, FromString&& from_string_func)
		{
			//either : T
			// or	 : T, T, T, T, T
			// or	 : [T, T, T, T, T, T]

			using value_type = typename T::value_type;

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

			if (std::any_of(std::begin(elements), std::end(elements), [](auto&& s) {
				return s.empty() //return true if the str is empty or consists of spaces
					|| std::all_of(std::begin(s), std::end(s), [](auto&& c) {return c == ' '; });
				}))
			{
				return T{};
			}

			constexpr auto custom_from_string = std::is_invocable_r_v<value_type, FromString, std::string_view>;

			//convert each one into T
			std::vector<value_type> out;
			out.reserve(size(elements));
			for (const auto& e : elements)
			{
				if constexpr (custom_from_string)
					out.emplace_back(std::invoke(from_string_func, e));
				else
				{
					std::ignore = from_string_func;
					out.emplace_back(from_string<value_type>(e));
				}
			}

			return out;
		}
	}

	template<typename T, typename FromString>
	T vector_from_string(std::string_view str, FromString&& func)
	{
		static_assert(std::is_invocable_r_v<typename T::value_type, FromString, std::string_view>,
			"vector_from_string(str, from_string): from_string must accept a std::string_view");
		return detail::vector_from_string_impl<T>(str, std::forward<FromString>(func));
	}

	template<typename T>
	T vector_from_string(std::string_view str)
	{
		return detail::vector_from_string_impl<T, nullptr_t>(str, nullptr);
	}
}
