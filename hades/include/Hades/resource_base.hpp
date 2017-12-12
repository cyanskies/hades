#ifndef HADES_RESOURCE_BASE_HPP
#define HADES_RESOURCE_BASE_HPP

#include <functional>

#include "Hades/Types.hpp"
#include "Hades/UniqueId.hpp"

//resource primatives

namespace YAML
{
	class Node;
}

namespace hades
{
	namespace data
	{
		class data_manager;

		//TODO: move the exceptions to data.hpp

		//the requested resource doesn't exist
		class resource_null : public std::runtime_error
		{
		public:
			using std::runtime_error::runtime_error;
		};

		//the requested resource isn't of the type it is claimed to be
		class resource_wrong_type : public std::runtime_error
		{
		public:
			using std::runtime_error::runtime_error;
		};
	}

	namespace resources
	{
		using parserFunc = std::function<void(data::UniqueId mod, const YAML::Node& node, data::data_manager*)>;

		struct resource_base
		{
			virtual ~resource_base() {}

			virtual void load(data::data_manager*) {}

			data::UniqueId id;
			//the file this resource should be loaded from
			//the path of the mod for resources that are parsed and not loaded
			types::string source;
			//the mod that the resource was most recently specified in
			//not nessicarily the only mod to specify this resource.
			data::UniqueId mod = data::UniqueId::Zero;
			bool loaded = false;
		};

		template<class T>
		struct resource_type : public resource_base
		{
			using loaderFunc = std::function<void(resource_base*, data::data_manager*)>;

			resource_type(loaderFunc loader = nullptr) : _resourceLoader(loader) {}

			virtual ~resource_type() {}

			void load(data::data_manager*);
			//the actual resource
			T value;
		protected:
			loaderFunc _resourceLoader;
		};
	}
}

#endif // !HADES_RESOURCE_BASE_HPP
