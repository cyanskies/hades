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

		template<class T>
		struct Property : public Property_Base
		{
			explicit Property(const T &value);
			std::shared_ptr<std::atomic<T> > value;

			virtual std::string to_string();
		};

		template<>
		struct Property<std::string>;

		class String
		{
		public:
			explicit String(std::string string);

			std::string load();
			void store(std::string string);

		private:
			std::mutex _stringMutex;
			std::string _data;
		};
	}

	// ==================
	// ---Main Console---
	// ==================
	class Console_String;
	class VerbPred;
	typedef std::list<Console_String> ConsoleStringBuffer;
	template<class T>
	using ConsoleVariable = std::shared_ptr<const std::atomic<T>>;

	class Console
	{
	public:
		enum Console_String_Verbosity { NORMAL, ERROR, WARNING };

		Console() : recentOutputPos(0) {}

		typedef std::function<bool(std::string)> Console_Function;

		bool registerFunction(const std::string &identifier, Console_Function func);

		template<class T>
		bool set(const std::string &identifier, const T &value);

		template<class T>
		ConsoleVariable<T> getValue(const std::string &var);

		bool runCommand(const std::string &command);
		void echo(const std::string &message, const Console_String_Verbosity verbosity = NORMAL);
		void echo(const Console_String &message);

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

	

	// =================
	// -Console Strings-
	// =================

	class Console_String
	{
	private:
		std::string Message;
		Console::Console_String_Verbosity Verb;
	public:
		Console_String(const std::string &message, const Console::Console_String_Verbosity &verb = Console::NORMAL) : Message(message), Verb(verb)
		{}

		const std::string Text() const { return Message; }
		const Console::Console_String_Verbosity Verbosity() const { return Verb; }
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