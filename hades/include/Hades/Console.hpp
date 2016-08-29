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

// =========
// Type Base
// =========
namespace hades
{
	//enum Console_Type {FUNCTION, BOOL, INT, FLOAT, STRING};
	namespace detail
	{
		struct Console_Type_Base
		{
			Console_Type_Base() : type(typeid(Console_Type_Base)) {}
			virtual ~Console_Type_Base() {}

			virtual std::string to_string() = 0;

			std::type_index type;
		};

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

		template<class T>
		struct Console_Type_Variable : public Console_Type_Base
		{
			explicit Console_Type_Variable(const T &value);
			std::shared_ptr<std::atomic<T> > value;

			virtual std::string to_string();
		};

		template<>
		struct Console_Type_Variable<std::string> : public Console_Type_Base
		{
			explicit Console_Type_Variable(const std::string &value) : value(new String(value))
			{}
			std::shared_ptr<String> value;

			virtual std::string to_string()
			{
				return value->load();
			}
		};

		struct Console_Type_Function : public Console_Type_Base
		{
			std::function<bool(std::string)> function;

			virtual std::string to_string();
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
    //#undef ERROR //what is this, and why is it here FIXME

	class Console
	{
	private:
		mutable std::mutex _consoleVariableMutex;
		mutable std::mutex _consoleBufferMutex;
		std::map<std::string, std::shared_ptr<detail::Console_Type_Base> > TypeMap;
		std::vector<Console_String> TextBuffer;
		int recentOutputPos;

	protected:
		bool GetValue(const std::string &var, std::shared_ptr<detail::Console_Type_Base> &out) const;
		bool SetVariable(const std::string &identifier, const std::string &value); //for unknown types stored as string, passed in by RunCommand
		void EchoVariable(const std::string &identifier);

	public:
		enum Console_String_Verbosity {NORMAL, ERROR, WARNING};

		Console() : recentOutputPos(0) {}

		bool registerFunction(const std::string &identifier, std::function<bool(std::string)> func);

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

#endif //SETH_CONSOLE_HPP
