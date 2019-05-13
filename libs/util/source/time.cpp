#include "hades/time.hpp"

#include "hades/utility.hpp"

namespace hades
{
	float normalise_time(time_point t, time_duration d) noexcept
	{
		if (d == time_duration::zero())
			return 0.f;

		const auto duration_point = time_point{ d };

		//reduce t untill is is less than one duration;
		while (t > duration_point)
			t -= d;

		return  static_cast<float>(t.time_since_epoch().count()) / static_cast<float>(d.count());
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
