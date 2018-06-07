#ifndef HADES_CONSOLE_HPP
#define HADES_CONSOLE_HPP

#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <typeindex>
#include <variant>

#include "Hades/Logging.hpp"
#include "Hades/Properties.hpp"
#include "Hades/Types.hpp"
#include "Hades/System.hpp"

//Console is a unique engine object for holding game wide properties
//it also exposes dev commands
//it also provides logging functionality

// =========
// Type Base
// =========
namespace hades
{
	namespace detail
	{
		using Property = std::variant<console::property_int, console::property_bool,
			console::property_float, console::property_str>;

		const auto to_string_lamb = [](auto &&val)->types::string {
			return to_string(val->load());
		};
	}

	// ==================
	// ---Main Console---
	// ==================
	using Console_String = console::string;
	class VerbPred;
	using ConsoleStringBuffer = console::output_buffer;
	template<class T>
	using ConsoleVariable = console::property<T>;

	class Console final : public console::logger, public console::properties, public console::system
	{
	public:
		using Console_Function = console::function;
		using Console_Function_No_Arg = console::function_no_argument;
		using Console_String_Verbosity = console::logger::log_verbosity;
		
		Console() : recentOutputPos(0) {}

		virtual ~Console() {}

		bool add_function(std::string_view identifier, Console_Function func, bool replace) override;
		bool add_function(std::string_view identifier, Console_Function_No_Arg func, bool replace) override;
		void erase_function(std::string_view identifier) override;

		template<class T>
		void setValue(std::string_view identifier, const T &value);

		void set(std::string_view, types::int32) override;
		void set(std::string_view, float) override;
		void set(std::string_view, bool) override;
		void set(std::string_view, std::string_view) override;

		template<class T>
		ConsoleVariable<T> getValue(std::string_view var);

		console::property_int getInt(std::string_view) override;
		console::property_float getFloat(std::string_view) override;
		console::property_bool getBool(std::string_view) override;
		console::property_str getString(std::string_view) override;

		bool run_command(const command &command) override;

		console::command_history_list command_history() const override;

		void echo(std::string_view message, Console_String_Verbosity verbosity = logger::log_verbosity::normal);
		void echo(const Console_String &message) override;

		bool exists(const std::string &command) const;
		void erase(const std::string &command);

		ConsoleStringBuffer get_new_output(Console_String_Verbosity maxVerbosity = logger::log_verbosity::normal) override;
		ConsoleStringBuffer get_output(Console_String_Verbosity maxVerbosity = logger::log_verbosity::normal) override;

	protected:
		//returns false if var was not found; true if out contains the requested value
		bool GetValue(std::string_view var, detail::Property &out) const;
		//for unknown types stored as string, passed in by RunCommand
		bool SetVariable(std::string_view identifier, const std::string &value); 
		void EchoVariable(std::string_view identifier);
		void DisplayVariables(std::vector<std::string_view> args);
		void DisplayFunctions(std::vector<std::string_view> args);

	private:
		mutable std::mutex _consoleVariableMutex;
		mutable std::mutex _consoleFunctionMutex;
		mutable std::mutex _consoleBufferMutex;
		mutable std::mutex _historyMutex;
		using ConsoleFunctionMap = std::map<types::string, Console_Function>;
		ConsoleFunctionMap _consoleFunctions;
		using ConsoleVariableMap = std::map<types::string, detail::Property>;
		ConsoleVariableMap _consoleVariables;
		std::vector<Console_String> TextBuffer;
		console::command_history_list _commandHistory;
		int recentOutputPos;
	};

	// ==================
	// -----Predicate----
	// ==================

	class VerbPred
	{
	private:
		Console::Console_String_Verbosity CurrentVerb;
	public:
		void SetVerb(const Console::Console_String_Verbosity &verb) { CurrentVerb = verb; }
		bool operator()(const Console_String &string) const { return string.verbosity() > CurrentVerb; }
	};
}//hades

#include "detail/Console.inl"

#endif //HADES_CONSOLE_HPP