#include "Hades/Console.hpp"

#include <map>
#include <list>
#include <sstream>
#include <string>

#ifdef _DEBUG
#include <iostream>
#endif

#include "Hades/Utility.hpp"

//==============
// Console
//==============
namespace hades
{
	bool Console::GetValue(std::string_view var, std::shared_ptr<detail::Property_Base> &out) const
	{
		auto iter = TypeMap.find(types::string(var));
		if(iter == TypeMap.end())
			return false;
		else
			out = iter->second;
		return true;
	}

	bool Console::SetVariable(std::string_view identifier, const std::string &value)
	{
		//ditermine type;
		//first check if indentifer exists, and use it's type;
		//second nothing, don't let end users define variables, easy!
		//or register specific functions for defining variables dynamically
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
				try
				{
					//float
					if (var->type == typeid(float))
						ret = set<float>(identifier, types::stov<float>(value));
					//bool
					else if (var->type == typeid(bool))
						ret = set<bool>(identifier, types::stov<bool>(value));
					//string
					else if (var->type == typeid(types::string))
						ret = set<types::string>(identifier, value);
					//integer
					else if (var->type == typeid(types::int32))
						ret = set<types::int32>(identifier, types::stov<types::int32>(value));

					echo(to_string(identifier) + " " + value);
				}
				catch (std::invalid_argument&)
				{
					echo("Unsupported type for variable: " + to_string(identifier), ERROR);
				}
				catch (std::out_of_range&)
				{
					echo("Value is out of range for variable: " + to_string(identifier), ERROR);
				}
			}
		}
		else
			echo("Attemped to set undefined variable: " + to_string(identifier), WARNING);
		return ret;
	}

	void Console::EchoVariable(std::string_view identifier)
	{
		std::shared_ptr<detail::Property_Base> var;

		if(GetValue(identifier, var))
		{
			echo(to_string(identifier) + " " + var->to_string());
		}
	}

	void Console::DisplayVariables(std::string_view arg)
	{
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);

		std::vector<std::string_view> args;
		split(arg, ' ', std::back_inserter(args));
		
		std::vector<types::string> output;

		auto insert = [&output](const ConsoleVariableMap::value_type &v) {
			output.push_back(v.first + " " + v.second->to_string());
		};

		for (auto &var : TypeMap)
		{
			if (args.empty())
				insert(var);
			else
			{
				for (auto arg : args)
				{
					if (var.first.find(arg) 
						!= ConsoleVariableMap::value_type::first_type::npos)
					{
						insert(var);
						break;
					}
				}
			}
		}

		std::sort(output.begin(), output.end());
		for (auto &s : output)
			echo(s);
	}

	void Console::DisplayFunctions(std::string_view arg)
	{
		//split arg if it contains spaces
		std::vector<std::string_view> args;
		split(arg, ' ', std::back_inserter(args));

		std::vector<types::string> funcs;
		{
			std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
			std::transform(std::begin(_consoleFunctions), std::end(_consoleFunctions), std::back_inserter(funcs),
				[](const ConsoleFunctionMap::value_type &f) { return f.first; });
		}

		funcs.push_back("vars");
		funcs.push_back("funcs");

		//erase everthing we won't be listing
		funcs.erase(std::remove_if(std::begin(funcs), std::end(funcs),
			[&args](const types::string &f)->bool {
			if (args.empty()) return false;

			for (auto arg : args)
			{
				if (f.find(arg) != types::string::npos)
					return false;
			}

			return true;
		}), std::end(funcs));

		std::sort(std::begin(funcs), std::end(funcs));

		for (auto &s : funcs)
			echo(s);
	}

	bool Console::registerFunction(std::string_view identifier, std::function<bool(std::string)> func, bool replace)
	{
		//test to see the name hasn't been used for a variable
		{
			std::lock_guard<std::mutex> lock(_consoleVariableMutex);
			std::shared_ptr<detail::Property_Base> var;
			if (GetValue(identifier, var))
			{
				echo("Attempted definition of function: " + to_string(identifier) + ", but name is already used for a variable.", ERROR);
				return false;
			}
		}

		auto id_str = to_string(identifier);

		std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
		
		auto funcIter = _consoleFunctions.find(id_str);

		if (funcIter != _consoleFunctions.end() && !replace)
		{
			echo("Attempted multiple definitions of function: " + id_str, ERROR);
			return false;
		}

		_consoleFunctions[id_str] = func;

		return true;
	}

	void Console::eraseFunction(std::string_view identifier)
	{
		std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
		_consoleFunctions.erase(to_string(identifier));
	}

	void Console::set(std::string_view name, types::int32 val)
	{
		if (!set<types::int32>(name, val))
			throw console::property_wrong_type("name: " + std::string(name) + ", value: " + to_string(val));
	}

	void Console::set(std::string_view name, float val)
	{
		if(!set<float>(name, val))
			throw console::property_wrong_type("name: " + std::string(name) + ", value: " + to_string(val));
	}

	void Console::set(std::string_view name, bool val)
	{
		if (!set<bool>(name, val))
			throw console::property_wrong_type("name: " + std::string(name) + ", value: " + to_string(val));
	}

	void Console::set(std::string_view name, std::string_view val)
	{
		if (!set<types::string>(name, std::string(val)))
			throw console::property_wrong_type("name: " + std::string(name) + ", value: " + to_string(val));
	}

	console::property<types::int32> Console::getInt(std::string_view name)
	{
		return getValue<types::int32>(name);
	}

	console::property<float> Console::getFloat(std::string_view name)
	{
		return getValue<float>(name);
	}

	console::property<bool> Console::getBool(std::string_view name)
	{
		return getValue<bool>(name);
	}

	console::property_str Console::getString(std::string_view name)
	{
		std::shared_ptr<detail::Property_Base > out;
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		if (GetValue(name, out))
		{
			if (out->type == typeid(types::string))
			{
				auto value = std::static_pointer_cast<detail::Property<types::string> > (out);
				return value->value;
			}
			else
				return nullptr;
		}

		return nullptr;
	}

	bool Console::runCommand(std::string_view command)
	{
		//add to command history
		{
			std::lock_guard<std::mutex> lock(_historyMutex);
			auto entry = std::find(_commandHistory.begin(), _commandHistory.end(), command);

			if (entry != _commandHistory.end())
				_commandHistory.erase(entry);

			_commandHistory.push_back(to_string(command));
		}

		std::string_view identifier, value;

		if (int pos = command.find_first_of(" "); pos == std::string_view::npos)
		{
			identifier = command;
			value = "";
		}
		else
		{
			identifier = command.substr(0, pos);
			value = command.substr(pos + 1, command.length() - pos);
		}

		if (identifier == "vars")
		{
			DisplayVariables(value);
			return true;
		}
		else if (identifier == "funcs")
		{
			DisplayFunctions(value);
			return true;
		}

		std::function<bool(std::string)> function;
		bool isFunction = false;

		{
			std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
			auto funcIter = _consoleFunctions.find(to_string(identifier));

			if (funcIter != _consoleFunctions.end())
			{
				function = funcIter->second;
				isFunction = true;
			}		
		}

		if (auto str_value = to_string(value); isFunction)
		{
			echo(command);
			return function(str_value);
		}
		else
			SetVariable(identifier, str_value);

		return true;
	}

	std::vector<types::string> Console::getCommandHistory() const
	{
		std::lock_guard<std::mutex> lock(_historyMutex);
		return _commandHistory;
	}

	void Console::echo(std::string_view message, Console_String_Verbosity verbosity)
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

		eraseFunction(command);
	}

	ConsoleStringBuffer Console::get_new_output(Console_String_Verbosity maxVerbosity)
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

	ConsoleStringBuffer Console::get_output(Console_String_Verbosity maxVerbosity)
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