#include "hades/data.hpp"

#include <shared_mutex>

#include "hades/exceptions.hpp"

using namespace std::string_literals;

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

		data_manager::data_manager()
		{
			auto& m = _mod_stack.emplace_back();
			m.mod_info.id = make_unique_id();
			m.mod_info.name = "built-in";
			return;
		}

		bool data_manager::exists(unique_id id) const
		{
			return std::any_of(begin(_mod_stack), end(_mod_stack), [id](auto&& elm) {
				return std::any_of(begin(elm.resources), end(elm.resources), [id](auto&& res) {
					return res->id == id;
					});
				});
		}

		resources::resource_base *data_manager::get_resource(unique_id id, std::optional<unique_id> mod)
		{
			auto r = try_get_resource(id, mod);
			if (!r)
				throw resource_null{ "Failed to find resource for unique_id: "s + get_as_string(id) +
					(mod ? ", from mod: "s + get_as_string(*mod) : string{}) + "."s };

			return r;
		}

		resources::resource_base *data_manager::try_get_resource(unique_id id, std::optional<unique_id> mod) noexcept
		{
			// find the resource in the highest mod on the stack
			// starting at mod if specified
			const auto end = rend(_mod_stack);
			auto iter = mod ? std::find_if(rbegin(_mod_stack), end, [mod](auto&& elm) {
				return mod == elm.mod_info.id;
				}) : rbegin(_mod_stack);

			for (; iter != end; ++iter)
			{
				const auto res_end = std::end(iter->resources);
				auto res = std::find_if(begin(iter->resources), res_end, [id](auto&& res) noexcept {
					return res->id == id;
				});

				if (res != res_end)
					return res->get();
			}

			return nullptr;
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
					const auto& ids = link->get_reverse_links();
					const auto others = " Referenced from: "s + to_string(begin(ids), end(ids));
					LOGWARNING("Missing resource. "s + n.what() + others);
				}
			}

			return;
		}

		void data_manager::push_mod(mod m)
		{
			const auto end = std::end(_mod_stack);
			auto iter = std::find_if(begin(_mod_stack), end, [id = m.id](auto&& elm) {
				return id == elm.mod_info.id;
			});

			if (iter == end)
			{
				auto& elm = _mod_stack.emplace_back();
				elm.mod_info = std::move(m);
			}
			else
				iter->mod_info = std::move(m);
		}

		void data_manager::pop_mod()
		{
			assert(!empty(_mod_stack));
			_mod_stack.pop_back();
		}

		const mod& data_manager::get_mod(const unique_id id) const
		{
			auto ptr = const_cast<data_manager*>(this);
			return ptr->_get_mod(id).mod_info;
		}

		std::vector<data_manager::resource_storage*> data_manager::get_mod_stack()
		{
			auto out = std::vector<resource_storage*>{};
			std::transform(begin(_mod_stack), end(_mod_stack), back_inserter(out), [](auto&& res) {
				return &res;
				});
			return out;
		}

		data_manager::mod_storage& data_manager::_get_mod(const unique_id id)
		{
			const auto end = std::end(_mod_stack);
			auto m = std::find_if(begin(_mod_stack), end, [id](auto&& elm) {
				return elm.mod_info.id == id;
				});

			if (m == end)
				throw resource_error{ "Missing mod in data manager: "s + get_as_string(id) };

			return *m;
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

		const mod& get_mod(unique_id id)
		{
			auto [data, lock] = detail::get_data_manager_ptr_shared();
			std::ignore = lock;

			return data->get_mod(id);
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