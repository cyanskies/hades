#ifndef HADES_DATA_HPP
#define HADES_DATA_HPP

#include <map>
#include <shared_mutex>
#include <tuple>

#include "Hades/resource_base.hpp"
#include "Hades/UniqueId.hpp"

//Data provides thread safe access to the hades data manager.
namespace hades
{
	using UniqueId = data::UniqueId;
	const auto EmptyId = data::UniqueId::Zero;

	namespace data
	{
		class data_manager
		{
		public:
			virtual ~data_manager() = default;

			//returns true if the id has a resource associated with it
			bool exists(UniqueId id) const;

			//returns the resource associated with the id
			//returns the resource base class
			//throws resource_null if the id doesn't refer to a resource
			resources::resource_base *getResource(UniqueId id);

			//gets a non-owning ptr to the resource represented by id
			//if the reasource has not been loaded it will be loaded before returning
			template<class T>
			T *get(UniqueId id);

			//tag type to request that a resource not be lazy loaded
			struct no_load_t {};
			static const no_load_t no_load;

			//gets a non-owning ptr to the resource represented by id
			template<class T>
			T *get(UniqueId id, const no_load_t);

			//creates a resource with the value of ptr
			//and assigns it to the name id
			//throws resource_name_already_used if the id already refers to a resource
			template<class T>
			void set(UniqueId id, std::unique_ptr<T> ptr);

			//refresh functions request that the mentioned resources be pre-loaded
			virtual void refresh() = 0;
			virtual void refresh(data::UniqueId) = 0;
			template<class First, class Last>
			void refresh(First first, Last last);

			virtual types::string getAsString(UniqueId id) const = 0;
			virtual UniqueId getUid(std::string_view name) const = 0;
			virtual UniqueId getUid(std::string_view name) = 0;

		private:
			std::map<UniqueId, std::unique_ptr<resources::resource_base>> _resources;
		};

		namespace detail
		{
			void SetDataManagerPtr(data_manager* ptr);
			using exclusive_lock = std::unique_lock<std::shared_mutex>;
			using data_manager_exclusive = std::tuple<data_manager*, exclusive_lock>;
			data_manager_exclusive GetDataManagerPtrExclusive();
		}

		//===Shared functions, these can be used without blocking other shared threads===
		//returns true if there is a resource associated with this ID
		//this will return false even if a name has been associated with the id
		//this will only test that a resource structure has been created.
		bool Exists(UniqueId id);
		types::string GetAsString(UniqueId id);
		//returns UniqueId::Zero if the name cannot be assiciated with an id
		UniqueId GetUid(std::string_view name);

		//===Exclusive Functions: this will block all threads===
		//NOTE: Can throw hades::data::resource_null
		//		or hades::data::resource_wrong_type
		template<class T>
		const T* Get(UniqueId id);
	
		//refresh requests

		//returns the uid associated with this name
		//or associates the name with a new id if it isn't already
		UniqueId MakeUid(std::string_view name);
	}
}//hades

#include "Hades/Data.inl"

#endif //!HADES_DATA_HPP