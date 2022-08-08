#ifndef HADES_CONSOLE_HPP
#define HADES_CONSOLE_HPP

#include <functional>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <variant>

#include "hades/logging.hpp"
#include "hades/properties.hpp"
#include "hades/types.hpp"
#include "hades/system.hpp"

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
		using Property = std::variant<console::property_int, console::property_bool,
			console::property_float, console::property_str>;

		const auto to_string_lamb = [](auto &&val)->types::string {
			return to_string(val->load());
		};
	}

	// ==================
	// ---Main Console---
	// ==================
	using Console_String = console::string;
	using ConsoleStringBuffer = console::output_buffer;
	template<class T>
	using ConsoleVariable = console::property<T>;

	class Console final : public console::logger, public console::properties, public console::system
	{
	public:
		using Console_Function = console::function;
		using Console_Function_No_Arg = console::function_no_argument;
		using Console_String_Verbosity = console::logger::log_verbosity;
	
		bool add_function(std::string_view identifier, Console_Function func, bool replace, bool silent = false) override;
		bool add_function(std::string_view identifier, Console_Function_No_Arg func, bool replace, bool silent = false) override;
		void erase_function(std::string_view identifier) override;

		virtual void create(std::string_view, int32, bool) override;
		virtual void create(std::string_view, float, bool) override;
		virtual void create(std::string_view, bool, bool) override;
		virtual void create(std::string_view, std::string_view, bool) override;

		virtual void lock_property(std::string_view) override;

		template<class T>
		void setValue(std::string_view identifier, const T &value);

		void set(std::string_view, types::int32) override;
		void set(std::string_view, float) override;
		void set(std::string_view, bool) override;
		void set(std::string_view, std::string_view) override;

		template<class T>
		ConsoleVariable<T> getValue(std::string_view var);

		console::property_int getInt(std::string_view) override;
		console::property_float getFloat(std::string_view) override;
		console::property_bool getBool(std::string_view) override;
		console::property_str getString(std::string_view) override;

		bool run_command(const command &command) override;
		std::vector<std::string_view> get_function_names() const override;
		std::vector<std::string_view> get_property_names() const;
		console::command_history_list command_history() const override;

		void echo(std::string message, Console_String_Verbosity verbosity = logger::log_verbosity::normal);
		void echo(Console_String message) override;

		struct variable_t {};
		struct function_t {};

		static constexpr variable_t variable{};
		static constexpr function_t function{};

		bool exists(const std::string_view &command) const;
		bool exists(const std::string_view &command, variable_t) const;
		bool exists(const std::string_view &command, function_t) const;

		void erase(const std::string &command);

		ConsoleStringBuffer get_new_output() override;
		ConsoleStringBuffer get_output() override;
		ConsoleStringBuffer steal_output() noexcept override;

	private:
		//returns false if var was not found; true if out contains the requested value
		bool GetValue(std::string_view var, detail::Property &out) const;
		//for unknown types stored as string, passed in by RunCommand
		bool SetVariable(std::string_view identifier, const std::string &value); 
		void EchoVariable(std::string_view identifier);
		void DisplayVariables(std::vector<std::string_view> args);
		void DisplayFunctions(std::vector<std::string_view> args);

	private:
		template<class T>
		void _create_property(std::string_view identifier, T value, bool lock);

		template<class T>
		void _set_property(std::string_view identifier, T value);

		mutable std::mutex _consoleVariableMutex;
		mutable std::mutex _consoleFunctionMutex;
		mutable std::mutex _consoleBufferMutex;
		mutable std::mutex _historyMutex;

		struct function_struct
		{
			Console_Function func;
			bool silent;
		};
		using ConsoleFunctionMap = std::unordered_map<types::string, function_struct>;
		ConsoleFunctionMap _consoleFunctions;
		using ConsoleVariableMap = std::unordered_map<types::string, detail::Property>;
		ConsoleVariableMap _consoleVariables;
		std::vector<Console_String> TextBuffer;
		console::command_history_list _commandHistory;
		std::size_t recentOutputPos = 0u;
	};
}//hades

#include "detail/Console.inl"

#endif //HADES_CONSOLE_HPP