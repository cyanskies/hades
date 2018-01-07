#ifndef HADES_DATA_HPP
#define HADES_DATA_HPP

#include <shared_mutex>
#include <mutex>
#include <tuple>

#include "Hades/UniqueId.hpp"

//Data provides thread safe access to the hades data manager.
namespace hades
{
	using UniqueId = data::UniqueId;
	const auto EmptyId = data::UniqueId::Zero;

	namespace data
	{
		class data_manager;

		namespace detail
		{
			void SetDataManagerPtr(data_manager* ptr);
			using data_manager_exclusive = std::tuple<data_manager*, std::unique_lock<std::shared_mutex>>;
			data_manager_exclusive GetDataManagerPtrExclusive();
		}

		//Shared functions, these can be used without blocking other shared threads

		//returns true if there is a resource associated with this ID
		//this will return false even if a name has been associated with the id
		//this will only test that a resource structure has been created.
		bool Exists(UniqueId id);

		//NOTE: Can throw hades::data::resource_null
		//		or hades::data::resource_wrong_type
		template<class T>
		const T* Get(UniqueId id);
		types::string GetAsString(UniqueId id);

		//Exclusive Functions: this will block all threads

		//returns the uid associated with this name
		//or associates the name with a new id if it isn't already
		UniqueId GetUid(std::string_view name);
	}
}//hades

#include "Hades/Data.inl"

#endif //!HADES_DATA_HPP