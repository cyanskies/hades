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
	typedef std::list<Console_String> ConsoleStringBuffer;
	template<class T>
	using ConsoleVariable = console::property<T>;

	class Console : public console::logger, public console::properties
	{
	public:
		using Console_String_Verbosity = console::logger::LOG_VERBOSITY;
		
		Console() : recentOutputPos(0) {}

		virtual ~Console() {}

		typedef std::function<bool(std::string)> Console_Function;

		bool registerFunction(const std::string &identifier, Console_Function func);

		template<class T>
		bool set(const std::string &identifier, const T &value);

		virtual bool set(const types::string&, types::int32);
		virtual bool set(const types::string&, float);
		virtual bool set(const types::string&, bool);
		virtual bool set(const types::string&, const types::string&);

		template<class T>
		ConsoleVariable<T> getValue(const std::string &var);

		virtual console::property<types::int32> getInt(const types::string&);
		virtual console::property<float> getFloat(const types::string&);
		virtual console::property<bool> getBool(const types::string&);
		virtual console::property_str getString(const types::string&);

		bool runCommand(const std::string &command);
		void echo(const types::string &message, const Console_String_Verbosity verbosity = NORMAL);
		virtual void echo(const Console_String &message);

		bool exists(const std::string &command) const;
		void erase(const std::string &command);

		const ConsoleStringBuffer getRecentOutputFromBuffer(Console_String_Verbosity maxVerbosity = NORMAL);
		const ConsoleStringBuffer getAllOutputFromBuffer(Console_String_Verbosity maxVerbosity = NORMAL);

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
		std::map<std::string, Console_Function> _consoleFunctions;
		std::map<std::string, std::shared_ptr<detail::Property_Base> > TypeMap;
		std::vector<Console_String> TextBuffer;
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