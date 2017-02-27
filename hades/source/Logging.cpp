#include "Hades/Logging.hpp"

namespace hades
{
	namespace console
	{
		logger* log = nullptr;

		void echo(const string& val)
		{
			if (log)
				log->echo(val);
		}
	}

	types::string time()
	{
		return "now";
	}
}//hades