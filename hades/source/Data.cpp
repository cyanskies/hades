#include "Hades/Data.hpp"

#include <shared_mutex>

#include "Hades/exceptions.hpp"

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
				if (Ptr == nullptr)
					throw provider_unavailable("data_manager provider unavailable");

				lock_t lock(Mutex);
				return std::make_tuple(Ptr, std::move(lock));
			}

			using data_manager_shared = std::tuple<const data_manager*, std::shared_lock<std::shared_mutex>>;

			data_manager_shared GetDataManagerPtrShared()
			{
				if (Ptr == nullptr)
					throw provider_unavailable("data_manager provider unavailable");

				shared_lock_t lock(Mutex);
				return std::make_tuple(Ptr, std::move(lock));
			}
		}

		const data_manager::no_load_t data_manager::no_load;
		
		bool data_manager::exists(UniqueId id) const
		{
			return _resources.find(id) != std::end(_resources);
		}

		resources::resource_base *data_manager::getResource(UniqueId id)
		{
			auto res = _resources.find(id);
			if (res == std::end(_resources))
				throw resource_null("Failed to find resource for unique_id: " + getAsString(id));

			return res->second.get();
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

			return data->getAsString(id);
		}

		UniqueId GetUid(std::string_view name)
		{
			const data_manager* data = nullptr;
			shared_lock_t lock;

			std::tie(data, lock) = detail::GetDataManagerPtrShared();

			return data->getUid(name);
		}


		UniqueId MakeUid(std::string_view name)
		{
			data_manager* data = nullptr;
			lock_t lock;

			std::tie(data, lock) = detail::GetDataManagerPtrExclusive();

			return data->getUid(name);
		}
	}
}