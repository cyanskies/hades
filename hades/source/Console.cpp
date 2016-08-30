#include "Hades/Console.hpp"

#include <map>
#include <list>
#include <sstream>
#include <string>

#ifdef _DEBUG
#include <iostream>
#endif

//==============
// Console
//==============
namespace hades
{
	namespace detail
	{
		String::String(std::string string) : _data(string)
		{}

		std::string String::load()
		{
			std::lock_guard<std::mutex> mutex(_stringMutex);
			return _data;
		}

		void String::store(std::string string)
		{
			std::lock_guard<std::mutex> mutex(_stringMutex);
			_data = string;
		}
	}

	bool Console::GetValue(const std::string &var, std::shared_ptr<detail::Property_Base> &out) const
	{
		auto iter = TypeMap.find(var);
		if(iter == TypeMap.end())
			return false;
		else
			out = iter->second;
		return true;
	}

	bool Console::SetVariable(const std::string &identifier, const std::string &value)
	{
		//ditermine type;
		//first check if indentifer exists, and use it's type;
		//second nothing, don't let end users define variables, easy!
		//or register specific functions for deefining variables dynamically
		bool ret = false;

		std::shared_ptr<detail::Property_Base> var;
		if(GetValue(identifier, var))
		{
			if(value.empty())
			{
				EchoVariable(identifier);
			}
			else
			{
				using namespace types;

				//flaots
				if (var->type == typeid(float))
					ret = set(identifier, stov<float>(value));
				else if (var->type== typeid(double) )
					ret = set(identifier, stov<double>(value));
				else if (var->type == typeid(long double))
					ret = set(identifier, stov<long double>(value));
				//bool
				else if (var->type== typeid(bool) )
					ret = set(identifier, stov<bool>(value));
				//string
				else if (var->type == typeid(std::string))
					ret = set(identifier, value);
				//integers
				else if (var->type == typeid(int8))
					ret = set(identifier, stov<int8>(value));
				else if (var->type == typeid(uint8))
					ret = set(identifier, stov<uint8>(value));
				else if (var->type == typeid(int16))
					ret = set(identifier, stov<int16>(value));
				else if (var->type == typeid(uint16))
					ret = set(identifier, stov<uint16>(value));
				else if (var->type == typeid(int32))
					ret = set(identifier, stov<int32>(value));
				else if (var->type == typeid(uint32))
					ret = set(identifier, stov<uint32>(value));
				else if (var->type == typeid(int64))
					ret = set(identifier, stov<int64>(value));
				else if (var->type == typeid(uint64))
					ret = set(identifier, stov<uint64>(value));
				else
					//must be an integer type
					echo("Unsupported type for variable: " + identifier, ERROR);

				echo(identifier + " " + value);
			}
		}
		else
			echo("Attemped to set undefined variable: " + identifier, WARNING);
		return ret;
	}

	void Console::EchoVariable(const std::string &identifier)
	{
		std::shared_ptr<detail::Property_Base> var;

		if(GetValue(identifier, var))
		{
			echo(identifier + " " + var->to_string());
		}
	}

	bool Console::registerFunction(const std::string &identifier, std::function<bool(std::string)> func)
	{
		return false;

		/*std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		std::shared_ptr<detail::Property_Base> var;
		if(GetValue(identifier, var))
		{
			echo("Attempted multiple definitions of function: " + identifier, ERROR);
			return false;
		}
		else
		{
			std::shared_ptr<detail::Console_Type_Function> function = std::make_shared<detail::Console_Type_Function>();
			function->function = func;
			function->type = typeid(detail::Console_Type_Function); 
			TypeMap[identifier] = function;
			return true;
		}*/
	}

	bool Console::runCommand(const std::string &command)
	{
		return false;

		//std::string identifier, value;
		//int pos = command.find_first_of(" ");

		//if (pos == std::string::npos)
		//{
		//	identifier = command;
		//	value = "";
		//}
		//else
		//{
		//	identifier = std::string(command, 0, pos);
		//	value = std::string(command, ++pos, command.length() - pos);
		//}

		//detail::Console_Type_Function func;
		//bool isFunction = false;

		//{
		//	std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		//	std::shared_ptr<detail::Console_Type_Base> var;

		//	if (!GetValue(identifier, var))
		//	{
		//		echo("Command not found: \"" + identifier + "\"", NORMAL);
		//		return false;
		//	}
		//	else if (var->type == typeid(detail::Console_Type_Function))
		//	{
		//		assert(std::dynamic_pointer_cast<detail::Console_Type_Function>(var) != nullptr);
		//		isFunction = true;
		//		// TODO: we already check that the type is a function, 
		//		// we can save some time by doing a normal cast instead of dynamic
		//		func = *std::dynamic_pointer_cast<detail::Console_Type_Function>(var);
		//	}
		//	else
		//		SetVariable(identifier, value);
		//}

		//if (isFunction)
		//{
		//	echo(command);
		//	return func.function(value);
		//}

		return true;
	}

	void Console::echo(const std::string &message, const Console_String_Verbosity verbosity)
	{
		echo(Console_String(message, verbosity));
	}

	void Console::echo(const Console_String &message)
	{
		std::lock_guard<std::mutex> lock(_consoleBufferMutex);
		TextBuffer.push_back(message);

		#ifdef _DEBUG
			std::cerr << message.Text() << std::endl;
		#endif
	}

	bool Console::exists(const std::string &command) const
	{
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		std::shared_ptr<detail::Property_Base> var;
		if (GetValue(command, var))
			return true;

		return false;
	}

	void Console::erase(const std::string &command)
	{
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		std::shared_ptr<detail::Property_Base> var;
		if (TypeMap.erase(command) == 0)
			echo("Failed to erase variable: " + command);			
	}

	const ConsoleStringBuffer Console::getRecentOutputFromBuffer(Console_String_Verbosity maxVerbosity)
	{
		std::lock_guard<std::mutex> lock(_consoleBufferMutex);
		std::vector<Console_String>:: iterator iter = TextBuffer.begin() + recentOutputPos;

		ConsoleStringBuffer out(iter, TextBuffer.end());

		recentOutputPos = TextBuffer.size();

		VerbPred predicate;
		predicate.SetVerb(maxVerbosity);

		out.remove_if(predicate);
		return out;
	}

	const ConsoleStringBuffer Console::getAllOutputFromBuffer(Console_String_Verbosity maxVerbosity)
	{
		std::lock_guard<std::mutex> lock(_consoleBufferMutex);
		ConsoleStringBuffer out(TextBuffer.begin(), TextBuffer.end());

		recentOutputPos = TextBuffer.size();

		VerbPred predicate;
		predicate.SetVerb(maxVerbosity);

		out.remove_if(predicate);
		return out;
	}
}//hades