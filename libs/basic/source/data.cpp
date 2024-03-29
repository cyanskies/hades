#include "hades/data.hpp"

#include <algorithm>
#include <shared_mutex>

#include "hades/exceptions.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace hades
{
	namespace data
	{
		namespace detail
		{
			static data_manager* ptr = {};

			void set_data_manager_ptr(data_manager* new_ptr)
			{
				assert(ptr == nullptr);
				ptr = new_ptr;
			}

			data_manager& get_data_manager()
			{
				assert(ptr);
				return *ptr;
			}
		}

		constexpr auto built_in_name = "built-in"sv;

		data_manager::data_manager()
		{
			auto& m = _mod_stack.emplace_back(); 
			m.mod_info.id = make_unique_id();
			m.mod_info.name = built_in_name;
			return;
		}

		bool data_manager::exists(unique_id id) const
		{
			const auto lock = std::shared_lock{ _mut };
			return _exists(id);
		}

		bool data_manager::_exists(unique_id id) const
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

		resources::resource_base* data_manager::try_get_resource(unique_id id, std::optional<unique_id> mod) noexcept
		{
			const auto lock = std::shared_lock{ _mut };
			return _try_get_resource(id, mod);
		}

		// TODO: this function is duplicated in order to provide it for const access
		resources::resource_base *data_manager::_try_get_resource(unique_id id, std::optional<unique_id> mod) noexcept
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
					return *res;
			}

			return nullptr;
		}

		const resources::resource_base* data_manager::_try_get_resource(unique_id id, std::optional<unique_id> mod) const noexcept
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
						return *res;
				}

				return nullptr;
		}

		void data_manager::update_all_links()
		{
			const auto lock = std::scoped_lock{ _links_mut };
			for (auto& link : _resource_links)
			{
				try
				{
					link->update_link(*this);
				}
				catch (const resource_null& n)
				{
					using namespace std::string_literals;
					const auto& ids = link->get_reverse_links();
					auto id_msg = "["s;
					assert(!empty(ids));
					for (auto id : ids)
						id_msg += get_as_string(id) + ", ";

					id_msg.replace(size(id_msg) - 2, 2, "]");

					const auto others = " Referenced from: "s + id_msg;
					LOGWARNING("Missing resource. "s + n.what() + others);
				}
			}

			return;
		}

		void data_manager::erase(unique_id id, std::optional<unique_id> mod) noexcept
		{
			{
				const auto lock = std::unique_lock{ _mut };

				if (!mod)
					mod = _mod_stack.front().mod_info.id;

				auto& m = _get_mod(mod.value());

				const auto mend = end(m.resources);
				const auto res_iter = std::find_if(begin(m.resources), mend, [id](auto&& other) {
					return other->id == id;
					});

				if (res_iter == mend)
					return;

				//search all the groups to remove the resource
				for ([[maybe_unused]] auto& [name, group] : m.resources_by_type)
				{
					const auto gend = end(group);
					const auto iter = std::find_if(begin(group), gend, [id](auto&& other) {
						return other->id == id;
						});

					if (iter != gend)
					{
						group.erase(iter);
						break;
					}
				}

				m.resources.erase(res_iter);
			}

			//update links, since some of them may have been pointing to the
			// erased resource
			update_all_links();
			return;
		}

		void data_manager::push_mod(mod m)
		{
			const auto lock = std::unique_lock{ _mut };
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
			const auto lock = std::unique_lock{ _mut };
			assert(!empty(_mod_stack));
			auto& mod = _mod_stack.back();

			for (auto res : mod.resources)
			{
				assert(mod.mod_info.id == res->mod);
				abandon_refresh(res->id, mod.mod_info.id);
				_resources.erase(res->id, mod.mod_info.id);
			}

			_mod_stack.pop_back();
		}

		const mod& data_manager::get_mod(const unique_id id) const
		{
			const auto lock = std::shared_lock{ _mut };
			auto ptr = const_cast<data_manager*>(this);
			return ptr->get_mod(id);
		}

		mod& data_manager::get_mod(unique_id id)
		{
			const auto lock = std::shared_lock{ _mut };
			return _get_mod(id).mod_info;
		}

		const data_manager::resource_storage& data_manager::get_mod_data(unique_id id) const
		{
			const auto lock = std::shared_lock{ _mut };
			auto ptr = const_cast<data_manager*>(this);
			return ptr->_get_mod(id);
		}

		std::string_view data_manager::built_in_mod_name() noexcept
		{
			return built_in_name;
		}

		bool data_manager::is_built_in_mod(unique_id id) const noexcept
		{
			const auto lock = std::shared_lock{ _mut };
			assert(!empty(_mod_stack));
			return _mod_stack.front().mod_info.id == id;
		}

		std::size_t data_manager::get_mod_count() const noexcept
		{
			const auto lock = std::shared_lock{ _mut };
			return size(_mod_stack);
		}

		std::vector<data_manager::resource_storage*> data_manager::get_mod_stack()
		{
			const auto lock = std::shared_lock{ _mut };
			auto out = std::vector<resource_storage*>{};
			std::transform(begin(_mod_stack), end(_mod_stack), back_inserter(out), [](auto&& res) {
				return &res;
				});
			return out;
		}

		template<typename ResourcesStackCollection, typename Func>
		void for_each_resource_type(std::string_view resource_type, ResourcesStackCollection collection, std::optional<unique_id> mod, Func func)
		{
			const auto end = std::rend(collection);
			auto iter = mod ? std::find_if(rbegin(collection), end, [mod](const auto& elm) {
				return elm.mod_info.id == *mod;
				}) : collection.rbegin();

			if (iter == end)
				return;

			for (; iter != end; ++iter)
			{
				auto anim_groups = std::find_if(begin(iter->resources_by_type),
					std::end(iter->resources_by_type), [resource_type](const auto& res) {
						return res.first == resource_type;
					});

				if (anim_groups == std::end(iter->resources_by_type))
					break;

				std::ranges::for_each(anim_groups->second, func);
			}
			return;
		}

		std::vector<unique_id> data_manager::get_all_ids_for_type(std::string_view resource_type, std::optional<unique_id> mod) const
		{
			const auto lock = std::shared_lock{ _mut };
			auto out = std::vector<unique_id>{};

			for_each_resource_type(resource_type, _mod_stack, mod, [&out](auto &&elm){
				out.emplace_back(elm->id);
			});

			remove_duplicates(out);
			return out;
		}

		std::vector<std::string_view> data_manager::get_all_names_for_type(std::string_view resource_type, std::optional<unique_id> mod) const
		{
			const auto lock = std::shared_lock{ _mut };
			auto out = std::vector<std::string_view>{};

			for_each_resource_type(resource_type, _mod_stack, mod, [&](auto&& elm) {
				out.emplace_back(get_as_string(elm->id));
				});

			remove_duplicates(out);
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
			return detail::get_data_manager().exists(id);
		}

		const string& get_as_string(const unique_id id)
		{
			return detail::get_data_manager().get_as_string(id);
		}

		unique_id get_uid(std::string_view name)
		{
			return detail::get_data_manager().get_uid(name);
		}

		const mod& get_mod(unique_id id)
		{
			return detail::get_data_manager().get_mod(id);
		}

		std::vector<unique_id> get_all_ids_for_type(std::string_view type, std::optional<unique_id> mod)
		{
			return detail::get_data_manager().get_all_ids_for_type(type, mod);
		}

		std::vector<std::string_view> get_all_names_for_type(std::string_view type, std::optional<unique_id> mod)
		{
			return detail::get_data_manager().get_all_names_for_type(type, mod);
		}

		unique_id make_uid(std::string_view name)
		{
			return detail::get_data_manager().get_uid(name);
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