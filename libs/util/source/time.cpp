#include "hades/time.hpp"

#include <cmath>

#include "hades/string.hpp"

namespace hades
{
	float normalise_time(time_point t, time_duration d) noexcept
	{
		static_assert(std::is_same_v<time_point::duration, time_duration>);

		if (d == time_duration::zero())
			return 0.f;

		const auto total_time = static_cast<float>(t.time_since_epoch().count());
		const auto duration_point = static_cast<float>(d.count());

		const auto normal = std::fmod(total_time, duration_point);

		return normal / duration_point;
	}

	constexpr auto nanos_in_second = std::nano::den;
	constexpr auto nanos_in_milli = std::nano::den / std::milli::den;
	constexpr auto nanos_in_micro = std::nano::den / std::micro::den;

	std::pair<string, duration_ratio> duration_to_string(time_duration d)
	{
		using namespace std::string_literals;
		const auto count = d.count();

		if (count % nanos_in_second == 0)
		{
			const auto seconds_duration = time_cast<seconds>(d);
			const auto seconds_count = seconds_duration.count();
			return { to_string(seconds_count), duration_ratio::seconds };
		}

		if (count % nanos_in_milli == 0)
		{
			const auto milli_duration = time_cast<milliseconds>(d);
			const auto milli_count = milli_duration.count();
			return { to_string(milli_count), duration_ratio::millis };
		}

		if (count % nanos_in_micro == 0)
		{
			const auto micro_duration = time_cast<microseconds>(d);
			const auto micro_count = micro_duration.count();
			return { to_string(micro_count), duration_ratio::micros };
		}

		return { to_string(count), duration_ratio::nanos };
	}

	string duration_to_string(time_duration d, duration_ratio r, int units_after_decimal)
	{
		const auto duration_f = time_cast<basic_duration<float, time_duration::period>>(d);
		auto str = string{};
		switch (r)
		{
		case duration_ratio::seconds:
		{
			const auto dir = time_cast<seconds_float>(duration_f);
			// TODO: cpp20, we can remove the string trimming at the end of the function
			// //		and them merge the unit postfix into this switch statement
			// str = std::format("%.*f"s, units_after_decimal, dir.count());
			str = to_string(dir.count());
			break;
		}
		case duration_ratio::millis:
		{
			const auto dir = time_cast<milliseconds_float>(duration_f);
			str = to_string(dir.count());
			break;
		}
		case duration_ratio::micros:
		{
			const auto dir = time_cast<basic_duration<float, microseconds::period>>(duration_f);
			str = to_string(dir.count());
			break;
		}
		case duration_ratio::nanos:
			[[fallthrough]];
		default:
		{
			const auto dir = time_cast<basic_duration<float, nanoseconds::period>>(duration_f);
			str = to_string(dir.count());
		}
		}

		// trim down to correct accuracy
		auto deci = std::find(begin(str), end(str), '.');
		if (deci != end(str))
		{
			while (deci != end(str) && units_after_decimal-- > 0)
			{
				advance(deci, 1);
			}

			str.erase(next(deci), end(str));
		}

		using namespace std::string_literals;

		switch (r)
		{
		case duration_ratio::seconds:
			return str + "s"s;
		case duration_ratio::millis:
			return str + "ms"s;
		case duration_ratio::micros:
			return str + "us"s;
		case duration_ratio::nanos:
			[[fallthrough]];
		default:
			return str + "ns"s;
		}
	}

	string to_string(time_duration d)
	{
		using namespace std::string_literals;
		const auto [count_str, ratio] = duration_to_string(d);

		switch (ratio)
		{
		case duration_ratio::seconds:
			return count_str + "s"s;
		case duration_ratio::millis:
			return count_str + "ms"s;
		case duration_ratio::micros:
			return count_str + "us"s;
		case duration_ratio::nanos:
			[[fallthrough]];
		default:
			return count_str + "ns"s;
		}
	}

	time_duration duration_from_string(std::string_view s) noexcept
	{
		using namespace std::string_view_literals;
		constexpr auto nano_ext = "ns"sv;
		constexpr auto micro_ext = "us"sv;
		constexpr auto milli_ext = "ms"sv;
		constexpr auto second_ext = "s"sv;
		constexpr auto npos = std::string_view::npos;

		if (const auto ms_pos = s.find(milli_ext); ms_pos != npos)
		{
			const auto milli_count = from_string<milliseconds::rep>(s.substr(0, ms_pos));
			return milliseconds{ milli_count };
		}
		else if (const auto ns_pos = s.find(nano_ext); ns_pos != npos)
		{
			const auto nano_count = from_string<nanoseconds::rep>(s.substr(0, ns_pos));
			return nanoseconds{ nano_count };
		}
		else if (const auto us_pos = s.find(micro_ext); us_pos != npos)
		{
			const auto micro_count = from_string<microseconds::rep>(s.substr(0, us_pos));
			return microseconds{ micro_count };
		}
		else if (const auto s_pos = s.find(second_ext); s_pos != npos)
		{
			const auto second_count = from_string<seconds_float::rep>(s.substr(0, s_pos));
			//hades::seconds_float is a float, needs time_cast to cast safely
			return time_cast<time_duration>(seconds_float{ second_count });
		}

		//default with no extention is ms
		const auto ms_count = from_string<milliseconds::rep>(s);
		return milliseconds{ ms_count };
	}
}
