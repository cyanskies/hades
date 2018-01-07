#include "Hades/data_manager.hpp"

namespace hades
{
	namespace data
	{
		template<class T>
		const T* Get(UniqueId id)
		{
			auto [data, lock] = detail::GetDataManagerPtrExclusive();
			return data->get<T>(id);
		}
	}
}