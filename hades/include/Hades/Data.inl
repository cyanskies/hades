#include "Hades/data_manager.hpp"

namespace hades
{
	namespace data
	{
		template<class T>
		const T* Get(UniqueId id)
		{
			data_manager* data = nullptr;
			std::unique_lock<std::shared_mutex> lock;
			std::tie(data, lock) = detail::GetDataManagerPtrExclusive();
			return data->get<T>(id);
		}
	}
}