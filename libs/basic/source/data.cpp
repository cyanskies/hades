#include "hades/data.hpp"

#include <shared_mutex>

#include "hades/exceptions.hpp"

namespace hades
{
	namespace data
	{
		using lock_t = detail::exclusive_lock;
		using shared_lock_t = std::shared_lock<std::shared_mutex>;

		namespace detail
		{
			static std::shared_mutex data_mutex;
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

				lock_t lock(data_mutex);
				return std::make_tuple(ptr, std::move(lock));
			}

			using data_manager_shared = std::tuple<const data_manager*, shared_lock_t>;

			data_manager_shared get_data_manager_ptr_shared()
			{
				if (ptr == nullptr)
					throw provider_unavailable("data_manager provider unavailable");

				shared_lock_t lock(data_mutex);
				return std::make_tuple(ptr, std::move(lock));
			}
		}

		bool data_manager::exists(unique_id id) const
		{
			const auto func = [id](auto&& res) noexcept {
				return res->id == id;
			};

			return std::any_of(begin(_main.resources), end(_main.resources), func)
				|| std::any_of(begin(_leaf.resources), end(_leaf.resources), func);
		}

		resources::resource_base *data_manager::get_resource(unique_id id)
		{
			auto r = try_get_resource(id);
			if(!r)
				throw resource_null("Failed to find resource for unique_id: " + get_as_string(id));

			return r;
		}

		resources::resource_base *data_manager::try_get_resource(unique_id id) noexcept
		{
			const auto func = [id](auto&& res) noexcept {
				return res->id == id;
			};

			const auto leaf_end = end(_leaf.resources);
			const auto main_end = end(_main.resources);
			auto res = std::find_if(begin(_leaf.resources), leaf_end, func);
			if (res == leaf_end)
			{
				res = std::find_if(begin(_main.resources), main_end, func);

				if (res == main_end)
					return nullptr;
			}

			return res->get();
		}

		void data_manager::update_all_links()
		{
			for (auto& link : _resource_links)
			{
				try {
					link->update_link();
				}
				catch (const resource_null& n)
				{
					using namespace std::string_literals;
					LOGWARNING("A resource is storing a reference to a missing resource. "s + n.what());
				}
			}

			return;
		}

		bool exists(unique_id id)
		{
			const data_manager* data = nullptr;
			shared_lock_t lock;

			std::tie(data, lock) = detail::get_data_manager_ptr_shared();

			return data->exists(id);
		}

		string get_as_string(unique_id id)
		{
			auto [data, lock] = detail::get_data_manager_ptr_shared();
			std::ignore = lock;
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

	string to_string(unique_id value)
	{
		return data::get_as_string(value);
	}

	template<>
	unique_id from_string<unique_id>(std::string_view value)
	{
		return data::get_uid(value);
	}
}