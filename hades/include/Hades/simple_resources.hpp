#ifndef HADES_SIMPLERESOURCE_HPP
#define HADES_SIMPLERESOURCE_HPP

#include "Hades/data_manager.hpp"
#include "Hades/parallel_jobs.hpp"

//This header provides resources for built in types

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
		void parseSystem(data::UniqueId mod, YAML::Node& node, data::data_manager*);
		void loadSystem(resource_base* r, data::data_manager* dataman);

		struct texture : public resource_type<sf::Texture>
		{
			texture() : resource_type<sf::Texture>(loadTexture) {}

			using size_type = types::uint16;
			//max texture size for older hardware is 512
			//max size for modern hardware is 8192 or higher
			size_type width, height;
			bool smooth, repeat, mips;
		};

		struct string : public resource_type<types::string>
		{};

		//a system stores a job function
		struct system : public resource_type<parallel_jobs::job_function>
		{
			//if loaded from a manifest then it should be loaded from scripts
			//if it's provided by the application, then source is empty, and no laoder function is provided.
		};
	}
}

#endif //HADES_SIMPLERESOURCE_HPP
