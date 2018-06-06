#ifndef HADES_RESOURCE_BASE_HPP
#define HADES_RESOURCE_BASE_HPP

#include <functional>

#include "Hades/Types.hpp"
#include "hades/uniqueid.hpp"

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

		class resource_name_already_used : public std::runtime_error
		{
		public:
			using std::runtime_error::runtime_error;
		};
	}

	namespace resources
	{
		struct resource_base
		{
			virtual ~resource_base() {}
			resource_base() = default;
			resource_base(const resource_base &rhs) = delete;
			resource_base(resource_base &&rhs) = delete;
			resource_base &operator=(const resource_base &rhs) = delete;
			resource_base &operator=(resource_base &&rhs) = delete;

			virtual void load(data::data_manager*) = 0;

			unique_id id;
			//the file this resource should be loaded from
			//the path of the mod for resources that are parsed and not loaded
			types::string source;
			//the mod that the resource was most recently specified in
			//not nessicarily the only mod to specify this resource.
			unique_id mod = unique_id::zero;
			bool loaded = false;
		};

		template<class T>
		struct resource_type : public resource_base
		{
			using loaderFunc = std::function<void(resource_base*, data::data_manager*)>;

			resource_type() = default;
			resource_type(loaderFunc loader) : _resourceLoader(loader) {}

			virtual ~resource_type() {}

			void load(data::data_manager*) override;
			//the actual resource
			T value;
		protected:
			loaderFunc _resourceLoader;
		};
	}
}

#endif // !HADES_RESOURCE_BASE_HPP
