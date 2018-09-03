#ifndef HADES_DATA_HPP
#define HADES_DATA_HPP

#include <map>
#include <shared_mutex>
#include <tuple>

#include "hades/resource_base.hpp"
#include "hades/types.hpp"
#include "hades/uniqueid.hpp"

//Data provides thread safe access to the hades data manager.
namespace hades
{
	const auto empty_id = unique_id::zero;

	namespace data
	{
		class data_manager
		{
		public:
			virtual ~data_manager() = default;

			//returns true if the id has a resource associated with it
			bool exists(unique_id id) const;

			//returns the resource associated with the id
			//returns the resource base class
			//throws resource_null if the id doesn't refer to a resource
			resources::resource_base *get_resource(unique_id id);

			//gets a non-owning ptr to the resource represented by id
			//if the reasource has not been loaded it will be loaded before returning
			template<class T>
			T *get(unique_id id);

			//tag type to request that a resource not be lazy loaded
			struct no_load_t {};
			static const no_load_t no_load;

			//gets a non-owning ptr to the resource represented by id
			template<class T>
			T *get(unique_id id, const no_load_t);

			//creates a resource with the value of ptr
			//and assigns it to the name id
			//throws resource_name_already_used if the id already refers to a resource
			template<class T>
			void set(unique_id id, std::unique_ptr<T> ptr);

			//refresh functions request that the mentioned resources be pre-loaded
			virtual void refresh() = 0;
			virtual void refresh(unique_id) = 0;
			template<class First, class Last>
			void refresh(First first, Last last);

			virtual types::string get_as_string(unique_id id) const = 0;
			virtual unique_id get_uid(std::string_view name) const = 0;
			virtual unique_id get_uid(std::string_view name) = 0;

		private:
			std::map<unique_id, std::unique_ptr<resources::resource_base>> _resources;
		};

		namespace detail
		{
			void set_data_manager_ptr(data_manager* ptr);
			using exclusive_lock = std::unique_lock<std::shared_mutex>;
			//TODO: return data_manager& here instead; as user is not permitted to store the result
			using data_manager_exclusive = std::tuple<data_manager*, exclusive_lock>;
			data_manager_exclusive get_data_manager_exclusive_lock();
		}

		//===Shared functions, these can be used without blocking other shared threads===
		//returns true if there is a resource associated with this ID
		//this will return false even if a name has been associated with the id
		//this will only test that a resource structure has been created.
		bool exists(unique_id id);
		types::string get_as_string(unique_id id);
		//returns UniqueId::zero if the name cannot be assiciated with an id
		unique_id get_uid(std::string_view name);

		//===Exclusive Functions: this will block all threads===
		//NOTE: Can throw hades::data::resource_null
		//		or hades::data::resource_wrong_type
		template<class T>
		const T* get(unique_id id);
	
		//refresh requests

		//returns the uid associated with this name
		//or associates the name with a new id if it isn't already
		unique_id make_uid(std::string_view name);
	}

	template<>
	types::string to_string(unique_id value);
}//hades

#include "hades/detail/data.inl"

#endif //!HADES_DATA_HPP