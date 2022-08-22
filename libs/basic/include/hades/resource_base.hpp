#ifndef HADES_RESOURCE_BASE_HPP
#define HADES_RESOURCE_BASE_HPP

#include <vector>

#include "hades/exceptions.hpp"
#include "hades/logging.hpp" // for temp serialise func
#include "hades/string.hpp"
#include "hades/uniqueid.hpp"

//resource primatives

namespace hades
{
	namespace data
	{
		class data_manager;
		class writer;

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

			virtual void load(data::data_manager&) {}
			virtual void serialise(data::data_manager&, data::writer&) const
			{
				throw logic_error{ "resource_base::serialise base implementation should never be invoked" };
			}

			using unload_func = void(*)(resource_base&);
			using clone_func = std::unique_ptr<resource_base>(*)(const resource_base&);

			unique_id id;
			//the mod that the resource was most recently specified in
			//not nessicarily the only mod to specify this resource.
			unique_id mod = unique_id::zero;
			bool loaded = false;

			// the file to write this resource to(eg. ./terrain/terrain)
			string data_file;

			unload_func clear = nullptr;
			clone_func clone = nullptr;
		};

		template<class T>
		struct resource_type : public resource_base
		{
			// TODO: turn into virtual func
			using loader_func = void(*)(resource_type<T>&, data::data_manager&);

			resource_type() = default;
			// TODO: why do we do this with a loader func,
			//	since load is virtual we can just let users override it?
			resource_type(loader_func loader) : _resource_loader{ loader } {}

			void load(data::data_manager &d) override 
			{
				if (_resource_loader)
					std::invoke(_resource_loader, *this, d);
			}

			void serialise(data::data_manager&, data::writer&) const override
			{	
				using namespace std::string_literals;
				log_debug("No serialise function for: "s + typeid(T).name());
				return;
			}

			//the actual resource
			// NOTE: this is often a empty tag type
			T value;
			//the path for this resource
			hades::string source;
		private:
			loader_func _resource_loader = nullptr;
		};
	}

	template<typename T>
	constexpr bool is_resource_v = std::is_base_of_v<resources::resource_base, T>;
}

#endif // !HADES_RESOURCE_BASE_HPP
