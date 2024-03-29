#include "hades/logging.hpp"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

//secure lib is a microsoft only impl that provides the 
//same as the standard compiant LIB_EXT1 but with a different name
#define __STDC_WANT_SECURE_LIB__ 1
#define __STDC_WANT_LIB_EXT1__ 1
#include <ctime>

#include "hades/time.hpp"
#include "hades/utility.hpp"

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
		const auto elems = std::array{ &brace, &verb, &colon, &brace, &_time, &brace2, &colon,
			&_file, &colon, &_function, &colon, &line, &brace2,	&_message };

		auto out = types::string{};
		out.reserve(/*seperators*/8 + size(verb) + size(line)
			+ size(_time) + size(_file) + size(_function) + size(_message));

		for (const auto s : elems)
			out += *s;

		return out;
	}

	namespace console
	{		
		logger* log = nullptr;

		std::ostream& operator<<(std::ostream& os, const string& s)
		{
			//[verb]:[time]:[file:func():line] Message
			const auto log_verbosity = to_string(s.verbosity());
			os << '[' << log_verbosity << "]:["s << s.time() << 
				"]:["s << s.file() << ':' << s.func() << "():"s <<
				s.line() << ']' << s.text();
			return os;
		}

		void echo(string val)
		{
			if (log)
				log->echo(std::move(val));
			else
			{
				if (val.verbosity() == console::logger::log_verbosity::normal)
					std::cout << val.text() << '\n';
				else
					std::cerr << val.text() << '\n';
			}
		}

		void echo(hades::string str, logger::log_verbosity verb, std::source_location loc)
		{
			const auto file = std::filesystem::path{ loc.file_name() };
			echo(string{ std::move(str), hades::integer_clamp_cast<int>(loc.line()), loc.function_name(), file.filename().string(), hades::time(), verb});
			return;
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

		console::output_buffer copy_output()
		{
			if (log)
				return log->copy_output();

			return {};
		}

		console::output_buffer steal_output() noexcept
		{
			if (log)
				return log->steal_output();

			return console::output_buffer{};
		}

		void start_log()
		{
			if (log)
				log->start_log();
			return;
		}

		bool is_logging() noexcept
		{
			if (log)
				return log->is_logging();
			return false;
		}

		void stop_log()
		{
			if (log)
				log->stop_log();
			return;
		}

		void dump_log()
		{
			if (log)
				log->dump_log();
			return;
		}
	}

	static string time_to_string(const char* fmt)
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
		ss << std::put_time(&time, fmt);
		return ss.str();
	}

	string date()
	{
		return time_to_string("%Y%m%d");
	}

	string time()
	{
		return time_to_string("%T");
	}
}//hades
