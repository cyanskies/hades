#ifndef HADES_RESOURCE_BASE_HPP
#define HADES_RESOURCE_BASE_HPP

#include <functional>

#include "hades/types.hpp"
#include "hades/uniqueid.hpp"

//resource primatives

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

			virtual void load(data::data_manager&) = 0;

			unique_id id;
			//the file this resource should be loaded from
			//the path of the mod for resources that are parsed and not loaded
			types::string source;
			//the mod that the resource was most recently specified in
			//not nessicarily the only mod to specify this resource.
			unique_id mod = unique_id::zero;
			bool loaded = false;

		protected:
			resource_base() = default;
			resource_base(const resource_base &rhs) = default;
			resource_base(resource_base &&rhs) = default;
			resource_base &operator=(const resource_base &rhs) = default;
			resource_base &operator=(resource_base &&rhs) = default;
		};

		template<class T>
		struct resource_type : public resource_base
		{
			using loader_func = std::function<void(resource_type<T>&, data::data_manager&)>;
			//TODO: depricate this
			using old_loader_func = std::function<void(resource_base*, data::data_manager*)>;
			using loaderFunc = old_loader_func;

			resource_type() = default;
			resource_type(loader_func loader) : _resource_loader(loader) {}
			resource_type(old_loader_func loader) : _resource_loader([loader](resource_type<T> &r, data::data_manager &d) { return  loader(&r, &d); })
			{}

			virtual ~resource_type() {}

			void load(data::data_manager &d) override 
			{
				if (_resource_loader)
					_resource_loader(*this, d);
			}

			//the actual resource
			T value;
		private:
			loader_func _resource_loader;
		};

		struct mod_t {};

		struct mod : public resource_type<mod_t>
		{
			// source == the name of the archive containing the mode
			// dependencies: a list of mods this mod depends on
			std::vector<unique_id> dependencies,
				//names: unique id's provided by this mod
				names;

			types::string name;
			//value is unused
			//mod_t value
		};
	}
}

#endif // !HADES_RESOURCE_BASE_HPP
