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
		struct Property_Base 
		{
			Property_Base(std::type_index i) : type(i) {}
			virtual ~Property_Base() {}
			std::type_index type;

			virtual std::string to_string() = 0;
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

	class Console : public console::logger, public console::properties, public console::system
	{
	public:
		using Console_Function = console::function;
		using Console_String_Verbosity = console::logger::LOG_VERBOSITY;
		
		Console() : recentOutputPos(0) {}

		virtual ~Console() {}

		bool registerFunction(const std::string &identifier, Console_Function func, bool replace) override;

		template<class T>
		bool set(const std::string &identifier, const T &value);

		bool set(const types::string&, types::int32) override;
		bool set(const types::string&, float) override;
		bool set(const types::string&, bool) override;
		bool set(const types::string&, const types::string&) override;

		template<class T>
		ConsoleVariable<T> getValue(const std::string &var);

		console::property<types::int32> getInt(const types::string&) override;
		console::property<float> getFloat(const types::string&) override;
		console::property<bool> getBool(const types::string&) override;
		console::property_str getString(const types::string&) override;

		bool runCommand(const std::string &command) override;

		std::vector<types::string> getCommandHistory() const override;

		void echo(const types::string &message, const Console_String_Verbosity verbosity = NORMAL);
		void echo(const Console_String &message) override;

		bool exists(const std::string &command) const;
		void erase(const std::string &command);

		ConsoleStringBuffer get_new_output(Console_String_Verbosity maxVerbosity = NORMAL) override;
		ConsoleStringBuffer get_output(Console_String_Verbosity maxVerbosity = NORMAL) override;

	protected:
		bool GetValue(const std::string &var, std::shared_ptr<detail::Property_Base> &out) const;
		bool SetVariable(const std::string &identifier, const std::string &value); //for unknown types stored as string, passed in by RunCommand
		void EchoVariable(const std::string &identifier);
		void DisplayVariables();
		void DisplayFunctions();

	private:
		mutable std::mutex _consoleVariableMutex;
		mutable std::mutex _consoleFunctionMutex;
		mutable std::mutex _consoleBufferMutex;
		mutable std::mutex _histoyMutex;
		std::map<std::string, Console_Function> _consoleFunctions;
		std::map<std::string, std::shared_ptr<detail::Property_Base> > TypeMap;
		std::vector<Console_String> TextBuffer;
		std::vector<types::string> _commandHistory;
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
		bool operator()(const Console_String &string) const { return string.Verbosity() > CurrentVerb; }
	};
}//hades

#include "detail/Console.inl"

#endif //HADES_CONSOLE_HPP