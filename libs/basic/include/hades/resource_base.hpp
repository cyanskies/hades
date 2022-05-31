#ifndef HADES_RESOURCE_BASE_HPP
#define HADES_RESOURCE_BASE_HPP

#include <vector>

#include "hades/exceptions.hpp"
#include "hades/string.hpp"
#include "hades/uniqueid.hpp"

//resource primatives

namespace hades
{
	namespace data
	{
		class data_manager;

		//TODO: move the exceptions to data.hpp
		class resource_error : public runtime_error
		{
		public:
			using runtime_error::runtime_error;
		};

		//the requested resource doesn't exist
		class resource_null : public resource_error
		{
		public:
			using resource_error::resource_error;
		};

		//the requested resource isn't of the type it is claimed to be
		class resource_wrong_type : public resource_error
		{
		public:
			using resource_error::resource_error;
		};

		//unique_id is already associated with a resource
		class resource_name_already_used : public resource_error
		{
		public:
			using resource_error::resource_error;
		};
	}

	namespace resources
	{
		struct mod; //fwd

		struct resource_base
		{
			virtual ~resource_base() {}

			virtual void load(data::data_manager&) = 0;

			unique_id id;
			//the mod that the resource was most recently specified in
			//not nessicarily the only mod to specify this resource.
			unique_id mod = unique_id::zero; //TODO: make ptr

			//TODO: depricate this
			//the file this resource should be loaded from
			//the path of the mod for resources that are parsed and not loaded
			string source;
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
			using loader_func = void(*)(resource_type<T>&, data::data_manager&);
			using update_links_func = void(*)(resource_type<T>&);
			using unload_func = void(*)(resource_type<T>&); // reset all settings to default?
			using writer_func = void(*)(resource_type<T>&);

			resource_type() = default;
			resource_type(loader_func loader) : _resource_loader{ loader } {}

			void load(data::data_manager &d) override 
			{
				if (_resource_loader)
					std::invoke(_resource_loader, *this, d);
			}

			//TODO: deprecate this
			//the actual resource
			T value;
		private:
			loader_func _resource_loader = nullptr;
		};

		struct mod_t {};

		struct mod : public resource_type<mod_t>
		{
			// source == the name of the archive containing the mod
			// dependencies: a list of mods this mod depends on
			std::vector<unique_id> dependencies,
				//names: unique id's provided by this mod
				names;

			string name;
			//value is unused
			//mod_t value
		};
	}

	template<typename T>
	constexpr bool is_resource_v = std::is_base_of_v<resources::resource_base, T>;
}

#endif // !HADES_RESOURCE_BASE_HPP
