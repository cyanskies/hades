#ifndef HADES_LOGGING_HPP
#define HADES_LOGGING_HPP

#include <list>

#include "hades/types.hpp"
#include "hades/utility.hpp"

//Interface and functions for global logging.
//a logger should impliment the interface and then a ptr assigned to hades::log to direct all logging to that logger
//logging will be skipped if hades::log is nullptr;

namespace hades
{
	namespace console
	{
		class string;

		using output_buffer = std::list<string>;
		//logging interface
		//this is implimented by hades::Console
		class logger
		{
		public:
			enum class log_verbosity { normal, error, warning };

			virtual ~logger() {}
			
			virtual void echo(const string&) = 0;

			virtual output_buffer get_new_output(log_verbosity max_verbosity) = 0;
			virtual output_buffer get_output(log_verbosity max_verbosity) = 0;
		};

		class string
		{
		public:
			explicit string(std::string_view message, const logger::log_verbosity &verb = logger::log_verbosity::normal) : _message(message), _verb(verb)
			{}

			string(std::string_view message, int line, std::string_view function, std::string_view file, std::string_view time,
				const logger::log_verbosity &verb = logger::log_verbosity::error)
				: _message(message), _verb(verb), _line(line), _function(function), _time(time)
			{}

			const types::string text() const { return _message; }
			const logger::log_verbosity verbosity() const { return _verb; }

			operator types::string() const { return "[" + _time + "]: " + _file + _function + to_string(_line) + "\n" + _message; }

		private:
			types::string _message;
			logger::log_verbosity _verb;
			int _line = -1;
			types::string _function, _file, _time;
		};

		//TODO: set log ptr function, so log is hidden from users
		extern logger *log;

		void echo(const string&);

		console::output_buffer new_output(console::logger::log_verbosity max_verbosity = console::logger::log_verbosity::normal);
		console::output_buffer output(console::logger::log_verbosity max_verbosity = console::logger::log_verbosity::normal);
	}

	types::string time();

	/* Need a standard way to get line number and filename without macros
	Waiting for std::source_location
	void log();
	void log_warning();
	void log_error();
	*/
}//hades


//logging macros, 
//these use the global console ptr, 
//they can be used to easily log with timestamps and function names/line numbers

#define HADESLOG(Message_, Verbosity_)					\
		hades::console::echo(hades::console::string(	\
			Message_,									\
			__LINE__,									\
			__func__,									\
			__FILE__,									\
			hades::time(),								\
			Verbosity_									\
	));													\

//Standard information log, for requst responses 
//or inforamtional messages
#define LOG(Message_)									\
	HADESLOG(Message_,									\
		hades::console::logger::log_verbosity::normal	\
	);

//Error log, indicates that a requested
//action failed to complete
#define LOGERROR(Message_)								\
	HADESLOG(Message_,									\
		hades::console::logger::log_verbosity::error	\
	);

//Warning log, for incorrect behaviour, any error which
//recovers becomes a warning, represents correctness and
//performace problems
#define LOGWARNING(Message_)							\
	HADESLOG(Message_,									\
		hades::console::logger::log_verbosity::warning	\
	);

#endif //HADES_LOGGING_HPP
