#ifndef HADES_LOGGING_HPP
#define HADES_LOGGING_HPP

#include <list>

#include "Hades/Types.hpp"

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
			#undef ERROR
			enum LOG_VERBOSITY { NORMAL, ERROR, WARNING };

			virtual ~logger() {}
			
			virtual void echo(const string&) = 0;

			virtual output_buffer get_new_output(LOG_VERBOSITY maxVerbosity) = 0;
			virtual output_buffer get_output(LOG_VERBOSITY maxVerbosity) = 0;
		};

		class string
		{
		private:
			types::string Message;
			logger::LOG_VERBOSITY Verb;
			int Line;
			types::string Function, File, Time;

		public:
			explicit string(std::string_view message, const logger::LOG_VERBOSITY &verb = logger::NORMAL) : Message(message), Verb(verb)
			{}

			string(std::string_view message, int line, std::string_view function, std::string_view file, std::string_view time, const logger::LOG_VERBOSITY &verb = logger::NORMAL)
				: Message(message), Verb(verb), Line(line), Function(function), Time(time)
			{}

			const types::string Text() const { return Message; }
			const logger::LOG_VERBOSITY Verbosity() const { return Verb; }

			operator types::string() const { return "[" + Time + "]: " + File + Function + to_string(Line) + "\n" + Message; }
		};

		extern logger *log;

		void echo(const string&);

		console::output_buffer new_output(console::logger::LOG_VERBOSITY maxVerbosity = console::logger::LOG_VERBOSITY::NORMAL);
		console::output_buffer output(console::logger::LOG_VERBOSITY maxVerbosity = console::logger::LOG_VERBOSITY::NORMAL);
	}

	types::string time();
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

#define LOG(Message_)									\
	HADESLOG(Message_,									\
		hades::console::logger::LOG_VERBOSITY::NORMAL	\
	);

#define LOGERROR(Message_)								\
	HADESLOG(Message_,									\
		hades::console::logger::LOG_VERBOSITY::ERROR	\
	);

#define LOGWARNING(Message_)							\
	HADESLOG(Message_,									\
		hades::console::logger::LOG_VERBOSITY::WARNING	\
	);

#endif //HADES_LOGGING_HPP
