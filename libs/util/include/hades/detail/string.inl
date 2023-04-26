#include "hades/string.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <functional>
#include <sstream>
#include <vector>

#include "hades/types.hpp"
#include "hades/utility.hpp"

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

		if (start == end)
			return in;

		return in.substr(start, end - start + 1);
	}

	template<typename T>
		requires (!string_type<T>)
	string to_string(T value)
	{
		using namespace std::string_literals;
		if constexpr (std::is_same_v<T, bool>)
			return value ? "true"s : "false"s;
		else
		{
			auto cstr = std::array<char, 30>{};
			auto begin = cstr.data();
			auto end = begin + size(cstr);
			auto result = std::to_chars_result{};

			if constexpr (std::is_integral_v<T>)
				result = std::to_chars(begin, end, value);
			else if constexpr (std::is_floating_point_v<T>)
				result = std::to_chars(begin, end, value, std::chars_format::fixed, 2);
			else
				static_assert(always_false_v<T>, "hades::to_string: unsupported type");

			if (result.ec == std::errc{})
				return { begin, result.ptr };
			else if (result.ec == std::errc::value_too_large && result.ptr == end)
				return std::to_string(value);
			else
				return {};
		}
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

	namespace detail
	{
		constexpr auto to_table = std::array<unsigned char, 64>
		{ 
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
			'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
			'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
			'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
			'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
			'w', 'x', 'y', 'z', '0', '1', '2', '3',
			'4', '5', '6', '7', '8', '9', '+', '/'
		};

		constexpr auto from_table = std::array<unsigned char, 128>
		{
			0,  0,  0,  0,  0,  0,  0,  0,  // 0
			0,  0,  0,  0,  0,  0,  0,  0,  // 8
			0,  0,  0,  0,  0,  0,  0,  0,  // 16
			0,  0,  0,  0,  0,  0,  0,  0,  // 24
			0,  0,  0,  0,  0,  0,  0,  0,  // 32
			0,  0,  0,  62, 0,  0,  0,  63, // 40
			52, 53, 54, 55, 56, 57, 58, 59, // 48
			60, 61, 0,  0,  0,  0,  0,  0,  // 56
			0,  0,  1,  2,  3,  4,  5,  6,  // 64
			7,  8,  9,  10, 11, 12, 13, 14, // 72
			15, 16, 17, 18, 19, 20, 21, 22, // 80
			23, 24, 25, 0,  0,  0,  0,  0,  // 88
			0,  26, 27, 28, 29, 30, 31, 32, // 96
			33, 34, 35, 36, 37, 38, 39, 40, // 104
			41, 42, 43, 44, 45, 46, 47, 48, // 112
			49, 50, 51, 0,  0,  0,  0,  0   // 120
		};
	}

	template<typename Ty>
		requires std::is_trivially_copyable_v<Ty>
	std::string base64_encode(std::span<Ty> source)
	{
		constexpr auto type_size = sizeof(Ty);
		const auto len = size(source) * type_size;

		auto result = std::string((len + 2) / 3 * 4, '=');

		auto* p = reinterpret_cast<const unsigned char*>(std::data(source));
		char* str = &result[0];
		std::size_t j = 0, pad = len % 3;
		const std::size_t last = len - pad;

		for (std::size_t i = 0; i < last; i += 3)
		{
			int n = int(p[i]) << 16 | int(p[i + 1]) << 8 | p[i + 2];
			str[j++] = detail::to_table[n >> 18];
			str[j++] = detail::to_table[n >> 12 & 0x3F];
			str[j++] = detail::to_table[n >> 6 & 0x3F];
			str[j++] = detail::to_table[n & 0x3F];
		}
		if (pad)  /// Set padding
		{
			int n = --pad ? int(p[last]) << 8 | p[last + 1] : p[last];
			str[j++] = detail::to_table[pad ? n >> 10 & 0x3F : n >> 2];
			str[j++] = detail::to_table[pad ? n >> 4 & 0x03F : static_cast<std::array<unsigned char, 64Ui64>::size_type>(n) << 4 & 0x3F];
			str[j++] = pad ? detail::to_table[static_cast<std::array<unsigned char, 64Ui64>::size_type>(n) << 2 & 0x3F] : '=';
		}
		return result;
	}

	template<typename Ty>
		requires std::is_default_constructible_v<Ty> && std::is_trivially_copyable_v<Ty>
	std::vector<Ty> base64_decode(std::string_view source)
	{
		if (empty(source)) return {};

		const auto len = size(source);

		auto p = reinterpret_cast<const unsigned char*>(std::data(source));
		size_t j = 0,
			// NOTE: MSVC warning about | being possibly intended 
			//		instead of || is incorrect
			pad1 = len % 4 || p[len - 1] == '=',
			pad2 = pad1 && (len % 4 > 2 || p[len - 2] != '=');
		const size_t last = (len - pad1) / 4 << 2;
		const auto result_size = last / 4 * 3 + pad1 + pad2;
		auto result = std::vector<Ty>(result_size / sizeof(Ty), {});
		unsigned char* str = reinterpret_cast<unsigned char*>(std::data(result));

		for (size_t i = 0; i < last; i += 4)
		{
			int n = detail::from_table[p[i]] << 18 | detail::from_table[p[i + 1]] << 12 | detail::from_table[p[i + 2]] << 6 | detail::from_table[p[i + 3]];
			str[j++] = integer_cast<unsigned char>(n >> 16);
			str[j++] = n >> 8 & 0xFF;
			str[j++] = n & 0xFF;
		}
		if (pad1)
		{
			int n = detail::from_table[p[last]] << 18 | detail::from_table[p[last + 1]] << 12;
			str[j++] = integer_cast<unsigned char>(n >> 16);
			if (pad2)
			{
				n |= detail::from_table[p[last + 2]] << 6;
				str[j++] = n >> 8 & 0xFF;
			}
		}
		return result;
	}
}
