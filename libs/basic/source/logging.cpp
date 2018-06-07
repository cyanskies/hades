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

namespace hades
{
	namespace console
	{
		logger* log = nullptr;

		void echo(const string& val)
		{
			if (log)
				log->echo(val);
			else
			{
				if (val.verbosity() == console::logger::log_verbosity::normal)
					std::cout << val.text();
				else
					std::cerr << val.text();
			}
		}

		console::output_buffer new_output(console::logger::log_verbosity max_verbosity)
		{
			if (log)
				return log->get_new_output(max_verbosity);

			return console::output_buffer();
		}

		console::output_buffer output(console::logger::log_verbosity max_verbosity)
		{
			if (log)
				return log->get_output(max_verbosity);

			return console::output_buffer();
		}
	}

	types::string time()
	{
		const auto t = std::chrono::system_clock::now();
		const auto ct = std::chrono::system_clock::to_time_t(t);
		//Use the updated C version of localtime
		//for better threading security
		tm time;
		#if defined(__STDC_LIB_EXT1__) || defined(__STDC_SECURE_LIB__)
			localtime_s(&time, &ct);
		#else
			//updated c version unavailable, have to suffer with the C99 version
			auto time_ptr = localtime(&ct);
			time = *time_ptr;
		#endif
		//use stream to collect formatted time
		std::stringstream ss;
		ss << std::put_time(&time, "%T");
		return ss.str();
	}
}//hades
