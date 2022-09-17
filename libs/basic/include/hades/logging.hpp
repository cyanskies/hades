#ifndef HADES_LOGGING_HPP
#define HADES_LOGGING_HPP

#include <filesystem>
#include <source_location>
#include <vector>

#include "hades/string.hpp"

//Interface and functions for global logging.
//a logger should impliment the interface and then a ptr assigned to hades::log to direct all logging to that logger
//logging will be skipped if hades::log is nullptr;

namespace hades
{
	namespace console
	{
		// logging data is streamed to log when it reaches this count
		constexpr auto log_limit = 800;
		// data will be streamed out until only this many entries remain in the log buffer
		constexpr auto log_shrink_count = 500;

		class string;

		using output_buffer = std::vector<string>;
		//logging interface
		//this is implimented by hades::Console
		class logger
		{
		public:
			enum class log_verbosity { log, normal = log, error, warning, debug };

			virtual ~logger() noexcept = default;
			
			virtual void echo(string) = 0;

			virtual output_buffer get_new_output() = 0;
			virtual output_buffer get_output() = 0;
			virtual output_buffer copy_output() = 0;
			// The logging record will be emptied after calling steal
			virtual output_buffer steal_output() noexcept = 0;
			virtual void start_log() = 0;
			virtual bool is_logging() noexcept = 0;
			virtual void stop_log() = 0;
			// opens log with default path if loggin was off
			virtual void dump_log() = 0;
		};

		class string
		{
		public:
			explicit string(std::string_view message, const logger::log_verbosity &verb = logger::log_verbosity::normal) noexcept
				: _message{ message }, _verb{ verb }
			{}

			string(std::string_view message, int line, std::string function, std::string file, std::string time,
				const logger::log_verbosity &verb = logger::log_verbosity::error) noexcept
				: _message{ message }, _verb{ verb }, _line{ line },
				_function{ std::move(function) }, _file{ std::move(file) },
				_time{ std::move(time) }
			{}

			const hades::string& text() const { return _message; }
			const hades::string& func() const { return _function; }
			const hades::string& file() const { return _file; }
			const hades::string& time() const { return _time; }
			int line() const { return _line; }
			const logger::log_verbosity verbosity() const { return _verb; }

			operator hades::string() const;
		private:
			hades::string _message;
			logger::log_verbosity _verb;
			int _line = -1;
			hades::string _function, _file, _time;
		};

		std::ostream& operator<<(std::ostream& os, const string& s);

		//TODO: set log ptr function, so log is hidden from users
		extern logger *log;

		void echo(string);
		void echo(hades::string str, logger::log_verbosity verb, std::source_location loc);

		console::output_buffer new_output();
		console::output_buffer output();
		// same as output(); but doesn't update the new output mark
		console::output_buffer copy_output();
		console::output_buffer steal_output() noexcept;

		void start_log();
		bool is_logging() noexcept;
		void stop_log();
		void dump_log();
	}

	string date();
	string time();

	template<string_type String>
	void log(String msg, std::source_location loc = std::source_location::current())
	{
		if constexpr (std::is_same_v<string, String>)
			console::echo(std::move(msg), console::logger::log_verbosity::log, loc);
		else
			console::echo(to_string(msg), console::logger::log_verbosity::log, loc);
		return;
	}

	template<string_type String>
	void log_warning(String msg, std::source_location loc = std::source_location::current())
	{
		if constexpr (std::is_same_v<string, String>)
			console::echo(std::move(msg), console::logger::log_verbosity::warning, loc);
		else
			console::echo(to_string(msg), console::logger::log_verbosity::warning, loc);
		return;
	}

	template<string_type String>
	void log_error(String msg, std::source_location loc = std::source_location::current())
	{
		if constexpr (std::is_same_v<string, String>)
			console::echo(std::move(msg), console::logger::log_verbosity::error, loc);
		else
			console::echo(to_string(msg), console::logger::log_verbosity::error, loc); 
		return;
	}

	template<string_type String>
	void log_debug(String msg, std::source_location loc = std::source_location::current())
	{
		if constexpr (std::is_same_v<string, String>)
			console::echo(std::move(msg), console::logger::log_verbosity::debug, loc);
		else
			console::echo(to_string(msg), console::logger::log_verbosity::debug, loc); 
		return;
	}
}//hades


// deprecated: use the log_* functions above
//logging macros, 
//these use the global console ptr, 
//they can be used to easily log with timestamps and function names/line numbers
//Standard information log, for requst responses 
//or inforamtional messages
#define LOG(Message_)			\
	hades::log(Message_)

//Error log, indicates that a requested
//action failed to complete
#define LOGERROR(Message_)		\
	hades::log_error(Message_)

//Warning log, for incorrect behaviour, any error which
//recovers becomes a warning, represents correctness and
//performace problems
#define LOGWARNING(Message_)	\
	hades::log_warning(Message_)

//Debug log, like the normal log, but with an annoying amount of detail
#define LOGDEBUG(Message_)		\
	hades::log_debug(Message_)

#endif //HADES_LOGGING_HPP
