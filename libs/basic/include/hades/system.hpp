#ifndef HADES_SYSTEM_HPP
#define HADES_SYSTEM_HPP

#include <functional>
#include <vector>

#include "hades/string.hpp"

// system provides support for running command line's into the engines console
// also supports registering global functions for more complicated commands.

namespace hades
{
	using argument_list = std::vector<std::string_view>;

	struct command
	{
		command() {}
		command(std::string_view sv);
		command(std::string_view command, argument_list args) : request(command),
			arguments(args)
		{}

		std::string_view request;
		argument_list arguments;
	};

	bool operator==(const command& lhs, const command &rhs);

	string to_string(const command&);

	typedef std::vector<command> command_list;

	namespace console
	{
		struct previous_command
		{
			previous_command() = default;
			previous_command(command com);
			
			hades::string request;
			using argument_list = std::vector<hades::string>;
			argument_list arguments;
		};

		bool operator==(const previous_command& lhs, const previous_command &rhs);
		hades::string to_string(const previous_command &command);

		using command_history_list = std::vector<previous_command>;

		using function_no_argument = std::function<bool(void)>;
		using function = std::function<bool(const argument_list&)>;

		class system
		{
		public:
			virtual ~system() noexcept = default;

			//registers a function with the provided name, if replace = true, then any function with the same name will be replaced with this one
			// note: this should not overwrite properties with the same name
			virtual bool add_function(std::string_view, function func, bool replace, bool silent = false) = 0;
			virtual bool add_function(std::string_view, function_no_argument func, bool replace, bool silent = false) = 0;
			//removes a function with the provided name
			virtual void erase_function(std::string_view) = 0;

			//returns true if the command was successful
			virtual bool run_command(const command&) = 0;

			virtual std::vector<std::string_view> get_function_names() const = 0;

			//returns the history of unique commands
			//newest commands are at the back of the vector
			//and the oldest at the front
			virtual command_history_list command_history() const = 0;
		};

		//TODO: replace with set system_ptr
		extern system *system_object;

		bool add_function(std::string_view, function func, bool replace = false, bool silent = false);
		bool add_function(std::string_view, function_no_argument, bool replace = false, bool silent = false);
		void erase_function(std::string_view); 
		std::vector<std::string_view> get_function_names();
		bool run_command(const command&);
		command_history_list command_history();
	}
}//hades

#endif //HADES_SYSTEM_HPP
