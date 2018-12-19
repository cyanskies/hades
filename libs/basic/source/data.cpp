#include "hades/data.hpp"

#include <shared_mutex>

#include "hades/exceptions.hpp"

namespace hades
{
	namespace data
	{
		using lock_t = std::unique_lock<std::shared_mutex>;
		using shared_lock_t = std::shared_lock<std::shared_mutex>;

		namespace detail
		{
			static std::shared_mutex Mutex;
			static data_manager* ptr = nullptr;
			void set_data_manager_ptr(data_manager* new_ptr)
			{
				assert(ptr == nullptr);
				ptr = new_ptr;
			}
		
			data_manager_exclusive get_data_manager_exclusive_lock()
			{
				if (ptr == nullptr)
					throw provider_unavailable("data_manager provider unavailable");

				lock_t lock(Mutex);
				return std::make_tuple(ptr, std::move(lock));
			}

			using data_manager_shared = std::tuple<const data_manager*, std::shared_lock<std::shared_mutex>>;

			data_manager_shared get_data_manager_ptr_shared()
			{
				if (ptr == nullptr)
					throw provider_unavailable("data_manager provider unavailable");

				shared_lock_t lock(Mutex);
				return std::make_tuple(ptr, std::move(lock));
			}
		}

		bool data_manager::exists(unique_id id) const
		{
			return _resources.find(id) != std::end(_resources);
		}

		resources::resource_base *data_manager::get_resource(unique_id id)
		{
			auto res = _resources.find(id);
			if (res == std::end(_resources))
				throw resource_null("Failed to find resource for unique_id: " + get_as_string(id));

			return res->second.get();
		}

		resources::resource_base *data_manager::try_get_resource(unique_id id)
		{
			auto res = _resources.find(id);
			return res == std::end(_resources) ? nullptr : res->second.get();
		}

		bool exists(unique_id id)
		{
			const data_manager* data = nullptr;
			shared_lock_t lock;

			std::tie(data, lock) = detail::get_data_manager_ptr_shared();

			return data->exists(id);
		}

		types::string get_as_string(unique_id id)
		{
			const data_manager* data = nullptr;
			shared_lock_t lock;

			std::tie(data, lock) = detail::get_data_manager_ptr_shared();

			return data->get_as_string(id);
		}

		unique_id get_uid(std::string_view name)
		{
			const data_manager* data = nullptr;
			shared_lock_t lock;

			std::tie(data, lock) = detail::get_data_manager_ptr_shared();

			return data->get_uid(name);
		}

		unique_id make_uid(std::string_view name)
		{
			data_manager* data = nullptr;
			lock_t lock;

			std::tie(data, lock) = detail::get_data_manager_exclusive_lock();

			std::ignore = lock;

			return data->get_uid(name);
		}
	}

	template<>
	types::string to_string<unique_id>(unique_id value)
	{
		return data::get_as_string(value);
	}

	template<>
	unique_id from_string<unique_id>(std::string_view value)
	{
		return data::get_uid(value);
	}
}