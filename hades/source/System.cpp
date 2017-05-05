#include "Hades/System.hpp"

namespace hades
{
	namespace console
	{
		system* system_object = nullptr;

		bool registerFunction(const types::string &identifier, function func, bool replace) 
		{
			if(system_object)
				return system_object->registerFunction(identifier, func, replace);

			return false;
		};

		bool runCommand(const types::string &command)
		{
			if(system_object)
				return system_object->runCommand(command);

			return false;
		}

		std::vector<types::string> getCommandHistory()
		{
			if (system_object)
				return system_object->getCommandHistory();

			return std::vector<types::string>();
		}
	}
}//hades
