#ifndef HADES_SYSTEM_HPP
#define HADES_SYSTEM_HPP

#include <functional>

#include "Hades/Types.hpp"

// system provides support for running command line's into the engines console
// also supports regestering global functions for more complicated commands.

namespace hades
{
	namespace console
	{
		using function = std::function<bool(types::string)>;


		class system
		{
		public:
			virtual ~system() {}

			//registers a function with the provided name, if replace = true, then any function with the same name will be replaced with this one
			// note: this should not overwrite properties with the same name
			virtual bool registerFunction(const types::string &identifier, function func, bool replace) = 0;

			//returns true if the command was successful
			virtual bool runCommand(const types::string &command) = 0;

			//TODO: get system output
		};

		extern system *system_object;

		bool registerFunction(const types::string &identifier, function func, bool replace = false);

		bool runCommand(const types::string &command);
	}
}//hades

#endif //HADES_SYSTEM_HPP