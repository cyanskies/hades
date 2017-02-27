#ifndef HADES_SIMPLERESOURCE_HPP
#define HADES_SIMPLERESOURCE_HPP

#include "Hades/data_manager.hpp"

namespace YAML
{
	class Node;
}

namespace hades
{
	namespace resources
	{
		void parseTexture(data::UniqueId mod, YAML::Node& node, data::data_manager*);
		void loadTexture(resource_base* r, data::data_manager* dataman);
		void parseString(data::UniqueId mod, YAML::Node& node, data::data_manager*);

		struct string : public resource_type<types::string>
		{
			string() : resource_type<types::string>([](resource_base*, data::data_manager*) {}) {}
		};

		struct texture : public resource_type<sf::Texture>
		{
			texture() : resource_type<sf::Texture>(loadTexture) {}

			using size_type = types::uint16;
			//max texture size for older hardware is 512
			//max size for modern hardware is 8192 or higher
			size_type width, height;
			bool smooth, repeat, mips;
		};
	}
}

#endif //HADES_SIMPLERESOURCE_HPP
