#ifndef HADES_SIMPLERESOURCE_HPP
#define HADES_SIMPLERESOURCE_HPP

#include "Hades/DataManager.hpp"

namespace YAML
{
	class Node;
}

namespace hades
{
	namespace resources
	{
		void parseTexture(data::UniqueId mod, YAML::Node& node, data::data_manager*);
		void parseString(data::UniqueId mod, YAML::Node& node, data::data_manager*);
	}
}

#endif //HADES_SIMPLERESOURCE_HPP
