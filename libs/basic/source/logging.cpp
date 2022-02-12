#include "hades/logging.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

//secure lib is a microsoft only impl that provides the 
//same as the standard compiant LIB_EXT1 but with a different name
#define __STDC_WANT_SECURE_LIB__ 1
#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>

#include "hades/time.hpp"

namespace hades
{
	using namespace std::string_literals;

	static string to_string(console::logger::log_verbosity v) noexcept
	{
		using verb = console::logger::log_verbosity;
		switch (v)
		{
		case verb::log: return "log"s;
		case verb::error: return "error"s;
		case verb::warning: return "warning"s;
		case verb::debug: return "debug"s;
		}
		return "unknown"s;
	}

	console::string::operator types::string() const
	{
		const auto brace = "["s;
		const auto colon = ":"s;
		const auto brace2 = "]"s;
		const auto verb = to_string(_verb);
		const auto line = to_string(_line);
		const auto elems = std::array{ &brace, &verb, &colon, &_time, &colon,
			&_file, &colon, &_function, &colon, &line, &brace2,	&_message };

		auto out = types::string{};
		out.reserve(/*seperators*/6 + size(verb) + size(line)
			+ size(_time) + size(_file) + size(_function) + size(_message));

		for (const auto s : elems)
			out += *s;

		return out;
	}

	namespace console
	{		
		logger* log = nullptr;

		void echo(string val)
		{
			if (log)
				log->echo(std::move(val));
			else
			{
				if (val.verbosity() == console::logger::log_verbosity::normal)
					std::cout << val.text();
				else
					std::cerr << val.text();
			}
		}

		console::output_buffer new_output()
		{
			if (log)
				return log->get_new_output();

			return console::output_buffer{};
		}

		console::output_buffer output()
		{
			if (log)
				return log->get_output();

			return console::output_buffer{};
		}
	}

	string time()
	{
		const auto t = wall_clock::now();
		const auto ct = wall_clock::to_time_t(t);
		//Use the updated C version of localtime
		//for better threading security
		tm time;
		#if defined(__STDC_LIB_EXT1__) || defined(__STDC_SECURE_LIB__)
			localtime_s(&time, &ct);
		#else
			//updated c version unavailable, have to suffer with the C99 version
			time = *localtime(&ct);
		#endif
		//use stream to collect formatted time
		std::stringstream ss;
		ss << std::put_time(&time, "%T");
		return ss.str();
	}
}//hades
