#include "Hades/Data.hpp"

#include <shared_mutex>

#include "Hades/data_manager.hpp"

namespace hades
{
	namespace data
	{
		using lock_t = std::unique_lock<std::shared_mutex>;
		using shared_lock_t = std::shared_lock<std::shared_mutex>;

		namespace detail
		{
			static std::shared_mutex Mutex;
			static data_manager* Ptr = nullptr;
			void SetDataManagerPtr(data_manager* ptr)
			{
				assert(Ptr == nullptr);
				Ptr = ptr;
			}
		
			data_manager_exclusive GetDataManagerPtrExclusive()
			{
				assert(Ptr);
				lock_t lock(Mutex);
				return std::make_tuple(Ptr, std::move(lock));
			}

			using data_manager_shared = std::tuple<const data_manager*, std::shared_lock<std::shared_mutex>>;

			data_manager_shared GetDataManagerPtrShared()
			{
				assert(Ptr);
				shared_lock_t lock(Mutex);
				return std::make_tuple(Ptr, std::move(lock));
			}
		}

		
		bool Exists(UniqueId id)
		{
			const data_manager* data = nullptr;
			shared_lock_t lock;

			std::tie(data, lock) = detail::GetDataManagerPtrShared();

			return data->exists(id);
		}

		types::string GetAsString(UniqueId id)
		{
			const data_manager* data = nullptr;
			shared_lock_t lock;

			std::tie(data, lock) = detail::GetDataManagerPtrShared();

			return data->as_string(id);
		}

		UniqueId GetUid(const types::string &name)
		{
			data_manager* data = nullptr;
			lock_t lock;

			std::tie(data, lock) = detail::GetDataManagerPtrExclusive();

			return data->getUid(name);
		}
	}
}