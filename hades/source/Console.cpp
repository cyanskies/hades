#include "Hades/Console.hpp"

#include <map>
#include <list>
#include <sstream>
#include <string>

#ifndef NDEBUG
#include <iostream>
#endif

#include "hades/exceptions.hpp"
#include "hades/utility.hpp"

//==============
// Console
//==============
namespace hades
{
	bool Console::GetValue(std::string_view var, detail::Property &out) const
	{
		const auto iter = _consoleVariables.find(to_string(var));
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
						using T = std::decay_t<decltype(arg)>::element_type::value_type;
						set(identifier, from_string<T>(value));
					}, var);

					LOG(to_string(identifier) + " " + to_string(value));
				}
				catch (std::invalid_argument&)
				{
					LOGERROR("Unsupported type for variable: " + to_string(identifier));
				}
				catch (std::out_of_range&)
				{
					LOGERROR("Value is out of range for variable: " + to_string(identifier));
				}
			}
		}
		else
			LOGERROR("Attemped to set undefined variable: " + to_string(identifier));
		return ret;
	}

	void Console::EchoVariable(std::string_view identifier)
	{
		detail::Property var;

		if(GetValue(identifier, var))
			LOG(to_string(identifier) + " " + std::visit(detail::to_string_lamb, var));
	}

	void Console::DisplayVariables(std::vector<std::string_view> args)
	{
		const std::lock_guard<std::mutex> lock(_consoleVariableMutex);

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
			LOG(s);
	}

	void Console::DisplayFunctions(std::vector<std::string_view> args)
	{
		std::vector<types::string> funcs;
		{
			const std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
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
			LOG(s);
	}

	bool Console::add_function(std::string_view identifier, console::function func, bool replace)
	{
		//test to see the name hasn't been used for a variable
		{
			std::lock_guard<std::mutex> lock(_consoleVariableMutex);
			detail::Property var;
			if (GetValue(identifier, var))
			{
				LOGERROR("Attempted definition of function: " + to_string(identifier) + ", but name is already used for a variable.");
				return false;
			}
		}

		const auto id_str = to_string(identifier);

		const std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
		
		const auto funcIter = _consoleFunctions.find(id_str);

		if (funcIter != _consoleFunctions.end() && !replace)
		{
			LOGERROR("Attempted multiple definitions of function: " + id_str);
			return false;
		}

		_consoleFunctions[id_str] = func;

		return true;
	}

	bool Console::add_function(std::string_view identifier, Console_Function_No_Arg func, bool replace) 
	{
		return add_function(identifier, [func](const argument_list&)->bool { return func(); }, replace);
	}

	void Console::erase_function(std::string_view identifier)
	{
		const std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
		_consoleFunctions.erase(to_string(identifier));
	}

	void Console::create(std::string_view s, int32 v)
	{
		_create_property(s, v);
	}

	void Console::create(std::string_view s, float v)
	{
		_create_property(s, v);
	}

	void Console::create(std::string_view s, bool v)
	{
		_create_property(s, v);
	}
	
	void Console::create(std::string_view s, std::string_view v)
	{
		_create_property(s, to_string(v));
	}

	void Console::set(std::string_view name, types::int32 val)
	{
		_set_property(name, val);
	}

	void Console::set(std::string_view name, float val)
	{
		_set_property(name, val);
	}

	void Console::set(std::string_view name, bool val)
	{
		_set_property(name, val);
	}

	void Console::set(std::string_view name, std::string_view val)
	{
		_set_property(name, to_string(val));
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
		const std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		if (GetValue(name, out))
		{
			using str = console::property_str;
			if (std::holds_alternative<str>(out))
				return std::get<str>(out);
		}

		return nullptr;
	}

	bool Console::run_command(const command &command)
	{
		//add to command history
		{
			const std::lock_guard<std::mutex> lock(_historyMutex);
			const auto entry = std::find(_commandHistory.begin(), _commandHistory.end(), command);

			if (entry != _commandHistory.end())
				_commandHistory.erase(entry);

			_commandHistory.push_back(command);
		}

		using namespace std::string_view_literals;

		if (command.request == "vars"sv)
		{
			DisplayVariables(command.arguments);
			return true;
		}
		else if (command.request == "funcs"sv)
		{
			DisplayFunctions(command.arguments);
			return true;
		}

		console::function function;
		bool isFunction = false;

		{
			const std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
			const auto funcIter = _consoleFunctions.find(to_string(command.request));

			if (funcIter != _consoleFunctions.end())
			{
				function = funcIter->second;
				isFunction = true;
			}		
		}

		if (isFunction)
		{
			LOG(to_string(command));
			return function(command.arguments);
		}
		else
			SetVariable(command.request, to_string(std::begin(command.arguments), std::end(command.arguments), " "sv));

		return true;
	}

	console::command_history_list Console::command_history() const
	{
		std::lock_guard<std::mutex> lock(_historyMutex);
		return _commandHistory;
	}

	void Console::echo(std::string message, Console_String_Verbosity verbosity)
	{
		echo(Console_String(std::move(message), verbosity));
	}

	void Console::echo(Console_String message)
	{
		const auto lock = std::scoped_lock{ _consoleBufferMutex };
		#ifndef NDEBUG // post to console window as well in debug mode
			std::cout << message.text() << "\n";
		#endif
		TextBuffer.emplace_back(std::move(message));
		return;
	}

	bool Console::exists(const std::string_view &command) const
	{
		return exists(command, variable) ||
			exists(command, function);
	}

	bool Console::exists(const std::string_view &command, variable_t) const
	{
		const std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		detail::Property var;
		if (GetValue(command, var))
			return true;

		return false;
	}

	bool Console::exists(const std::string_view &command, function_t) const
	{
		const std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
		const auto funcIter = _consoleFunctions.find(to_string(command));

		if (funcIter != _consoleFunctions.end())
			return true;

		return false;
	}

	void Console::erase(const std::string &command)
	{
		{
			const std::lock_guard<std::mutex> lock(_consoleVariableMutex);
			_consoleVariables.erase(command);
		}

		erase_function(command);
	}

	ConsoleStringBuffer Console::get_new_output(Console_String_Verbosity maxVerbosity)
	{
		const std::lock_guard<std::mutex> lock(_consoleBufferMutex);
		const std::vector<Console_String>:: iterator iter = TextBuffer.begin() + recentOutputPos;

		ConsoleStringBuffer out(iter, TextBuffer.end());

		recentOutputPos = TextBuffer.size();

		VerbPred predicate;
		predicate.SetVerb(maxVerbosity);

		out.remove_if(predicate);
		return out;
	}

	ConsoleStringBuffer Console::get_output(Console_String_Verbosity maxVerbosity)
	{
		const std::lock_guard<std::mutex> lock(_consoleBufferMutex);
		ConsoleStringBuffer out(TextBuffer.begin(), TextBuffer.end());

		recentOutputPos = TextBuffer.size();

		VerbPred predicate;
		predicate.SetVerb(maxVerbosity);

		out.remove_if(predicate);
		return out;
	}

	template<class T>
	void Console::_create_property(std::string_view identifier, T value)
	{
		//we can only store integral types in std::atomic
		static_assert(valid_console_type<T>::value, "Attempting to create an illegal property type");

		if (exists(identifier))
		{
			try
			{
				const auto prop = getValue<T>(identifier);
				if (prop->load_default() == value) //if we're creating again with the same settings; its fine
					return;
			}
			catch (const console::property_wrong_type&)
			{ /* error is covered by the exception thrown below */ }

			throw console::property_name_already_used{ "Cannot create property; name: " +
			to_string(identifier) + ", has already been used" };
		}

		std::lock_guard<std::mutex> lock(_consoleVariableMutex);

		auto var = console::detail::make_property<T>(value);

		_consoleVariables.emplace( to_string(identifier), std::move(var) );
	}

	template<class T>
	void Console::_set_property(std::string_view identifier, T value)
	{
		//we can only store integral types in std::atomic
		static_assert(valid_console_type<T>::value, "Attempting to set an illegal property type");

		if (!exists(identifier))
			throw console::property_missing{ "Cannot set property: " +
			to_string(identifier) + ", it doesn't exist" };

		if (exists(identifier, function))
			throw invalid_argument{ "Property set: " +
			to_string(identifier) + ", this name is used for a function" };

		detail::Property out;
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);

		const auto get = GetValue(identifier, out);

		if (!get)
			throw std::runtime_error{ "Failed to find property value" };

		using ValueType = std::decay_t<decltype(value)>;

		std::visit([identifier, &value](auto &&arg) {
			using U = std::decay_t<decltype(arg)>;
			using W = std::decay_t<U::element_type::value_type>;
			if constexpr (std::is_same_v<ValueType, W>)
				*arg = value;
			else
				throw console::property_wrong_type("name: " + to_string(identifier) + ", value: " + to_string(value));
		}, out);
	}
}//hades