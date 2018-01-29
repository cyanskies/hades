#include "Hades/System.hpp"

namespace hades
{
	namespace console
	{
		PreviousCommand::PreviousCommand(Command com) : command(com.command)
		{
			std::transform(std::begin(com.arguments), std::end(com.arguments), std::back_inserter(arguments), [](auto &&arg) {
				return hades::to_string(arg);
			});
		}

		bool operator==(const PreviousCommand& lhs, const PreviousCommand &rhs)
		{
			return lhs.command == rhs.command && lhs.arguments == rhs.arguments;
		}

		types::string to_string(const PreviousCommand &command)
		{
			return hades::to_string(command.command) + " " + hades::to_string(std::begin(command.arguments), std::end(command.arguments));
		}

		system* system_object = nullptr;

		bool RegisterFunction(std::string_view identifier, function func, bool replace)
		{
			if(system_object)
				return system_object->registerFunction(identifier, func, replace);

			return false;
		};

		void EraseFunction(std::string_view identifier)
		{
			if (system_object)
				system_object->eraseFunction(identifier);
		}

		bool RunCommand(const Command &command)
		{
			if(system_object)
				return system_object->runCommand(command);

			return false;
		}

		CommandHistory GetCommandHistory()
		{
			if (system_object)
				return system_object->getCommandHistory();

			return CommandHistory();
		}
	}
}//hades
