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
using namespace std::string_view_literals;

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
						using T = typename std::decay_t<decltype(arg)>::element_type::value_type;
						set(identifier, from_string<T>(value));
					}, var);

					LOG(to_string(identifier) + " " + to_string(value));
				}
				catch (console::property_error& e)
				{
					LOGERROR(e.what());
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
		auto vars = get_property_names();
		vars.erase(std::remove_if(begin(vars), end(vars), [&args](const auto v) {
			if (empty(args)) return false;

			for (auto& arg : args)
			{
				if (v.find(arg) != types::string::npos)
					return false;
			}

			return true;
		}), end(vars));

		std::sort(begin(vars), end(vars));
		for (auto &s : vars)
			LOG(s);
	}

	void Console::DisplayFunctions(std::vector<std::string_view> args)
	{
		auto funcs = get_function_names();

		//erase everthing we won't be listing
		funcs.erase(std::remove_if(std::begin(funcs), std::end(funcs),
			[&args](const auto f)->bool {
			if (args.empty()) return false;

			for (auto& arg : args)
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

	bool Console::add_function(std::string_view identifier, console::function func, bool replace, bool silent)
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

		_consoleFunctions[id_str] = { func, silent };

		return true;
	}

	bool Console::add_function(std::string_view identifier, Console_Function_No_Arg func, bool replace, bool silent) 
	{
		return add_function(identifier, [func](const argument_list&)->bool { return func(); }, replace, silent);
	}

	void Console::erase_function(std::string_view identifier)
	{
		const std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
		_consoleFunctions.erase(to_string(identifier));
	}

	void Console::create(std::string_view s, int32 v, bool l)
	{
		_create_property(s, v, l);
	}

	void Console::create(std::string_view s, float v, bool l)
	{
		_create_property(s, v, l);
	}

	void Console::create(std::string_view s, bool v, bool l)
	{
		_create_property(s, v, l);
	}
	
	void Console::create(std::string_view s, std::string_view v, bool l)
	{
		_create_property(s, to_string(v), l);
	}

	void Console::lock_property(std::string_view s)
	{
		detail::Property out;
		const auto lock = std::scoped_lock{ _consoleVariableMutex };
		if (GetValue(s, out))
		{
			std::visit([](auto& p) {
				p->lock(true);
				}, out);
		}
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

		console::function func;
		bool isFunction = false;
		bool silent = false;
		{
			const std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
			const auto funcIter = _consoleFunctions.find(to_string(command.request));

			if (funcIter != _consoleFunctions.end())
			{
				func = funcIter->second.func;
				silent = funcIter->second.silent;
				isFunction = true;
			}		
		}

		if (isFunction)
		{
			if(!silent)
				LOG(to_string(command));
			return func(command.arguments);
		}
		else
			SetVariable(command.request, to_string(std::begin(command.arguments), std::end(command.arguments), " "sv));

		return true;
	}

	std::vector<std::string_view> Console::get_function_names() const
	{
		auto funcs = std::vector<std::string_view>{"vars"sv, "funcs"sv};
		{
			const std::lock_guard<std::mutex> lock(_consoleFunctionMutex);
			funcs.reserve(size(funcs) + size(_consoleFunctions));
			std::transform(std::begin(_consoleFunctions), std::end(_consoleFunctions), std::back_inserter(funcs),
				[](const ConsoleFunctionMap::value_type& f)->std::string_view { return f.first; });
		}

		return funcs;
	}

	std::vector<std::string_view> Console::get_property_names() const
	{
		auto vars = std::vector<std::string_view>{};
		{
			const std::lock_guard<std::mutex> lock(_consoleVariableMutex);
			vars.reserve(size(_consoleVariables));
			std::transform(begin(_consoleVariables), end(_consoleVariables), std::back_inserter(vars),
				[](const ConsoleVariableMap::value_type& v)->std::string_view { return v.first; });
		}

		return vars;
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

	ConsoleStringBuffer Console::get_new_output()
	{
		const std::lock_guard<std::mutex> lock(_consoleBufferMutex);
		const std::vector<Console_String>:: iterator iter = TextBuffer.begin() + recentOutputPos;

		ConsoleStringBuffer out(iter, TextBuffer.end());

		recentOutputPos = TextBuffer.size();
		/*out.remove_if([maxVerbosity](auto& s) {
			return s.verbosity() > maxVerbosity;
			});*/

		return out;
	}

	ConsoleStringBuffer Console::get_output()
	{
		const std::lock_guard<std::mutex> lock(_consoleBufferMutex);
		ConsoleStringBuffer out(TextBuffer.begin(), TextBuffer.end());

		recentOutputPos = TextBuffer.size();

		/*out.remove_if([maxVerbosity](auto& s) {
			return s.verbosity() > maxVerbosity;
			});*/

		return out;
	}

	template<class T>
	void Console::_create_property(std::string_view identifier, T value, bool l)
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

		auto var = console::detail::make_property<T>(value, l);

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
			throw console::property_error{ "Cannot set property: " +
			to_string(identifier) + ", this name is used for a function" };

		detail::Property out;
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);

		const auto get = GetValue(identifier, out);

		if (!get)
			throw console::property_error{ "Failed to find property value" };

		using ValueType = std::decay_t<decltype(value)>;

		std::visit([identifier, &value](auto &&arg) {
			using U = std::decay_t<decltype(arg)>;
			using W = std::decay_t<U::element_type::value_type>;
			if constexpr (std::is_same_v<ValueType, W>)
			{
				if (arg->locked())
					throw console::property_locked{ "Unable to set locked property, name: " + to_string(identifier) };

				*arg = value;
			}
			else
				throw console::property_wrong_type("Unable to set property, was the wrong type. name: " + to_string(identifier) + ", value: " + to_string(value));
		}, out);
	}
}//hades