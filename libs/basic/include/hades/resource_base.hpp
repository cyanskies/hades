#ifndef HADES_RESOURCE_BASE_HPP
#define HADES_RESOURCE_BASE_HPP

#include <filesystem>
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
			virtual void serialise(const data::data_manager&, data::writer&) const = 0;
			//serialise(ostream&) will only be called if this returns true
			virtual bool serialise_source() const
			{
				return false;
			}

			virtual void serialise(std::ostream&) const {}

			using clone_func = std::unique_ptr<resource_base>(*)(const resource_base&);

			unique_id id;
			//the mod that the resource was most recently specified in
			//not nessicarily the only mod to specify this resource.
			unique_id mod = unique_id::zero;
			bool loaded = false;

			// the file to write this resource to(eg. ./terrain/terrain(.yaml))
			std::filesystem::path data_file;
			//the path for this resource within a mod(eg. ./terrain/dirt.png)
			// serialise(ostream&) will only be called if this isn't empty
			std::filesystem::path source;

			// recorded raw paths for the resource
			std::filesystem::path loaded_archive_path;
			std::filesystem::path loaded_path;

			clone_func clone = nullptr;
		};

		template<class T>
		struct resource_type : public resource_base
		{
			void serialise(const data::data_manager&, data::writer&) const override
			{	
				using namespace std::string_literals;
				log_debug("No serialise function for: "s + typeid(T).name());
				return;
			}

			//the actual resource
			// NOTE: this is often a empty tag type
			T value;
		};
	}

	template<typename T>
	constexpr bool is_resource_v = std::is_base_of_v<resources::resource_base, T>;

	template<typename T>
	concept Resource = is_resource_v<T>;
}

#endif // !HADES_RESOURCE_BASE_HPP
