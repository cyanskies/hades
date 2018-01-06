#include "Hades/System.hpp"

namespace hades
{
	namespace console
	{
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

		bool RunCommand(std::string_view command)
		{
			if(system_object)
				return system_object->runCommand(command);

			return false;
		}

		std::vector<types::string> GetCommandHistory()
		{
			if (system_object)
				return system_object->getCommandHistory();

			return std::vector<types::string>();
		}
	}
}//hades
