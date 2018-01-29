#ifndef HADES_SYSTEM_HPP
#define HADES_SYSTEM_HPP

#include <functional>
#include <vector>

#include "Hades/Main.hpp"
#include "Hades/Types.hpp"

// system provides support for running command line's into the engines console
// also supports registering global functions for more complicated commands.

namespace hades
{
	namespace console
	{
		struct PreviousCommand
		{
			PreviousCommand() = default;
			PreviousCommand(Command com);
			
			types::string command;
			using ArgumentList = std::vector<types::string>;
			ArgumentList arguments;
		};

		bool operator==(const PreviousCommand& lhs, const PreviousCommand &rhs);
		types::string to_string(const PreviousCommand &command);

		using CommandHistory = std::vector<PreviousCommand>;

		using function_no_argument = std::function<bool(void)>;
		using function = std::function<bool(const ArgumentList&)>;

		class system
		{
		public:
			virtual ~system() {}

			//registers a function with the provided name, if replace = true, then any function with the same name will be replaced with this one
			// note: this should not overwrite properties with the same name
			virtual bool registerFunction(std::string_view, function func, bool replace) = 0;
			virtual bool registerFunction(std::string_view, function_no_argument func, bool replace) = 0;
			//removes a function with the provided name
			virtual void eraseFunction(std::string_view) = 0;

			//returns true if the command was successful
			virtual bool runCommand(const Command&) = 0;

			//returns the history of unique commands
			//newest commands are at the back of the vector
			//and the oldest at the front
			virtual CommandHistory getCommandHistory() const = 0;
		};

		extern system *system_object;

		bool RegisterFunction(std::string_view, function func, bool replace = false);
		void EraseFunction(std::string_view);
		bool RunCommand(const Command&);
		CommandHistory GetCommandHistory();
	}
}//hades

#endif //HADES_SYSTEM_HPP
