#ifndef HADES_SIMPLERESOURCE_HPP
#define HADES_SIMPLERESOURCE_HPP

namespace YAML
{
	class Node;
}

namespace hades
{
	namespace data
	{
		class resource_base;
		class data_manager;
	}

	namespace resources
	{
		void parseTexture(const char* mod, YAML::Node& node);
		void loadTexture(data::resource_base*, data::data_manager*);
	}
}

#endif //HADES_SIMPLERESOURCE_HPP
