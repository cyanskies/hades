#include "Hades/Console.hpp"

#include <map>
#include <list>
#include <sstream>
#include <string>

#ifndef NDEBUG
#include <iostream>
#endif

#include "Hades/Utility.hpp"

//==============
// Console
//==============
namespace hades
{
	bool Console::GetValue(std::string_view var, detail::Property &out) const
	{
		auto iter = _consoleVariables.find(to_string(var));
		if(iter == _consoleVariables.end())
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

		detail::Property var;
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
					std::visit([identifier, &value, this](auto &&arg) {
						using T = std::decay_t<decltype(arg)>;
						using U = T::element_type::value_type;
						if constexpr(std::is_same_v<T, console::property<U>>)
							set(identifier, types::stov<U>(value));
						else if constexpr(std::is_same_v<T, console::property_str>)
							set(identifier, value);
					}, var);

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
			echo("Attemped to set undefined variable: " + to_string(identifier), ERROR);
		return ret;
	}

	void Console::EchoVariable(std::string_view identifier)
	{
		detail::Property var;

		if(GetValue(identifier, var))
		{
			echo(to_string(identifier) + " " + std::visit(detail::to_string_lamb, var));
		}
	}

	void Console::DisplayVariables(std::vector<std::string_view> args)
	{
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);

		std::vector<types::string> output;

		auto insert = [&output](const ConsoleVariableMap::value_type &v) {
			output.push_back(v.first + " " + std::visit(detail::to_string_lamb, v.second));
		};

		for (auto &var : _consoleVariables)
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

	void Console::DisplayFunctions(std::vector<std::string_view> args)
	{
		std::vector<types::string> funcs;
		{
			std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
			funcs.reserve(_consoleFunctions.size());
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

	bool Console::registerFunction(std::string_view identifier, console::function func, bool replace)
	{
		//test to see the name hasn't been used for a variable
		{
			std::lock_guard<std::mutex> lock(_consoleVariableMutex);
			detail::Property var;
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

	bool Console::registerFunction(std::string_view identifier, Console_Function_No_Arg func, bool replace) 
	{
		return registerFunction(identifier, [func](const ArgumentList&)->bool { return func(); }, replace);
	}

	void Console::eraseFunction(std::string_view identifier)
	{
		std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
		_consoleFunctions.erase(to_string(identifier));
	}

	void Console::set(std::string_view name, types::int32 val)
	{
		setValue(name, val);
	}

	void Console::set(std::string_view name, float val)
	{
		setValue(name, val);
	}

	void Console::set(std::string_view name, bool val)
	{
		setValue(name, val);
	}

	void Console::set(std::string_view name, std::string_view val)
	{
		setValue(name, to_string(val));
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
		detail::Property out;
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		if (GetValue(name, out))
		{
			using str = console::property_str;
			if (std::holds_alternative<str>(out))
				return std::get<str>(out);
		}

		return nullptr;
	}

	bool Console::runCommand(const Command &command)
	{
		//add to command history
		{
			std::lock_guard<std::mutex> lock(_historyMutex);
			auto entry = std::find(_commandHistory.begin(), _commandHistory.end(), command);

			if (entry != _commandHistory.end())
				_commandHistory.erase(entry);

			_commandHistory.push_back(command);
		}

		if (command.command == "vars")
		{
			DisplayVariables(command.arguments);
			return true;
		}
		else if (command.command == "funcs")
		{
			DisplayFunctions(command.arguments);
			return true;
		}

		console::function function;
		bool isFunction = false;

		{
			std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
			auto funcIter = _consoleFunctions.find(to_string(command.command));

			if (funcIter != _consoleFunctions.end())
			{
				function = funcIter->second;
				isFunction = true;
			}		
		}

		if (isFunction)
		{
			echo(to_string(command));
			return function(command.arguments);
		}
		else
			SetVariable(command.command, to_string(std::begin(command.arguments), std::end(command.arguments)));

		return true;
	}

	CommandList Console::getCommandHistory() const
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

		#ifndef NDEBUG
			std::cerr << message.Text() << std::endl;
		#endif
	}

	bool Console::exists(const std::string &command) const
	{
		{
			std::lock_guard<std::mutex> lock(_consoleVariableMutex);
			detail::Property var;
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
			_consoleVariables.erase(command);
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