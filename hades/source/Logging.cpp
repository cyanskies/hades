#include "Hades/Logging.hpp"

namespace hades
{
	namespace console
	{
		logger* log = nullptr;

		void echo(const string& val)
		{
			if (log)
				log->echo(val);
		}

		console::output_buffer new_output(console::logger::LOG_VERBOSITY maxVerbosity)
		{
			if (log)
				return log->get_new_output(maxVerbosity);

			return console::output_buffer();
		}
		console::output_buffer output(console::logger::LOG_VERBOSITY maxVerbosity)
		{
			if (log)
				return log->get_output(maxVerbosity);

			return console::output_buffer();
		}
	}

	types::string time()
	{
		return "now";
	}
}//hades