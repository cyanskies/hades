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

				//floats
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
		//test to see the name hasn't been used for a variable
		{
			std::lock_guard<std::mutex> lock(_consoleVariableMutex);
			std::shared_ptr<detail::Property_Base> var;
			if (GetValue(identifier, var))
			{
				echo("Attempted definition of function: " + identifier + ", but name is already used for a variable.", ERROR);
				return false;
			}
		}

		std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
		
		auto funcIter = _consoleFunctions.find(identifier);

		if (funcIter != _consoleFunctions.end())
		{
			echo("Attempted multiple definitions of function: " + identifier, ERROR);
			return false;
		}

		_consoleFunctions[identifier] = func;

		return true;
	}

	bool Console::runCommand(const std::string &command)
	{
		std::string identifier, value;
		int pos = command.find_first_of(" ");

		if (pos == std::string::npos)
		{
			identifier = command;
			value = "";
		}
		else
		{
			identifier = std::string(command, 0, pos);
			value = std::string(command, ++pos, command.length() - pos);
		}

		std::function<bool(std::string)> function;
		bool isFunction = false;

		{
			std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
			auto funcIter = _consoleFunctions.find(identifier);

			if (funcIter != _consoleFunctions.end())
			{
				function = funcIter->second;
				isFunction = true;
			}		
		}

		if (isFunction)
		{
			echo(command);
			return function(value);
		}
		else
			SetVariable(identifier, value);

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
		{
			std::lock_guard<std::mutex> lock(_consoleVariableMutex);
			std::shared_ptr<detail::Property_Base> var;
			if (GetValue(command, var))
				return true;
		}

		{
			std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
			auto funcIter = _consoleFunctions.find(command);

			if (funcIter != _consoleFunctions.end())
				return true;
		}

		return false;
	}

	void Console::erase(const std::string &command)
	{
		{
			std::lock_guard<std::mutex> lock(_consoleVariableMutex);
			TypeMap.erase(command);
		}

		{
			std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
			_consoleFunctions.erase(command);
		}
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