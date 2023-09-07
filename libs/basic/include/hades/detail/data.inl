#include "hades/data.hpp"

#include "hades/logging.hpp"
#include "hades/resource_base.hpp"
#include "hades/utility.hpp"

namespace hades
{
	namespace resources
	{
		template<typename T>
		inline void resource_link_type<T>::update_link(data::data_manager& d)
		{
			assert(id());
			_resource = std::invoke(_get, d, id());
			return;
		}

		template<typename T>
		std::vector<unique_id> get_ids(const std::vector<resource_link<T>>& res)
		{
			auto out = std::vector<unique_id>{};
			out.reserve(size(res));

			std::transform(begin(res), end(res), back_inserter(out), std::mem_fn(&resource_link<T>::id));
			return out;
		}
	}

	namespace data
	{
		namespace detail
		{
			template<Resource T>
			const T* get_no_load(data::data_manager& d, const unique_id id)
			{
				return d.get<T>(id, no_load);
			}

			template<Resource T>
			resources::resource_base* clone(const resources::resource_base& rb)
			{
				const auto& t = static_cast<const T&>(rb);
				return std::make_unique<T>(t);
			}
		}

		template<Resource T>
		T* data_manager::find_or_create(const unique_id target, std::optional<unique_id> mod, std::string_view group)
		{
			using namespace std::string_literals;
			using Type = std::decay_t<T>;

			Type* r = nullptr;

			if (target == unique_zero)
			{
				LOGERROR("Tried to create resource with hades::EmptyId, this id is reserved for unset Unique Id's, resource type was: "s + string(typeid(T).name()));
				return r;
			}

			const auto lock = std::unique_lock{ _mut };

			assert(!empty(_mod_stack));
			if (!mod)
				mod = _mod_stack.front().mod_info.id;

			if (!_exists(target))
			{
				auto res = T{};
				res.id = target;
				res.mod = *mod;
				res.data_file = _current_data_file();
				r = _set<Type>(std::move(res), group);
			}
			else
			{
				//get the one from the correct mod
				auto [get_r, e]= _try_get<Type>(target, no_load);

				if (!get_r)
				{
					if (e == get_error::resource_wrong_type)
					{
						//name is already used for something else, this cannnot be loaded
						LOGERROR("Failed to create "s + to_string(group) + " resource: "s + get_as_string(target) + " in mod : "s + get_as_string(*mod) +
							", name has already been used for a different resource type."s);
					}
				}
				else if (mod != get_r->mod)
				{
					auto res = *get_r;
					assert(res.id == target);
					res.mod = *mod;
					res.loaded = false;
					res.data_file = _current_data_file();
					r = _set<Type>(std::move(res), group);
				}
				else
					r = get_r;
			}

			return r;
		}

		template<Resource T>
		inline std::vector<T*> data_manager::find_or_create(const std::vector<unique_id>& target, std::optional<unique_id> mod, std::string_view group)
		{
			{
				const auto lock = std::shared_lock{ _mut };
				assert(!empty(_mod_stack));
				if (!mod)
					mod = _mod_stack.front().mod_info.id;
			}

			std::vector<T*> out;
			out.reserve(target.size());

			std::transform(std::begin(target), std::end(target), std::back_inserter(out), [&](const unique_id id) {
				return find_or_create<T>(id, mod, group);
			});

			return out;
		}

		template<Resource T>
		resources::resource_link<T> data_manager::make_resource_link(unique_id id, unique_id from,
			typename resources::resource_link_type<T>::get_func get)
		{
			const auto lock = std::scoped_lock{ _links_mut };
			return _make_resource_link<T>(id, from, get);
		}

		template<Resource T>
		std::vector<resources::resource_link<T>> data_manager::make_resource_link(const std::vector<unique_id>& ids,
			const unique_id from, const typename resources::resource_link_type<T>::get_func getter)
		{
			auto out = std::vector<resources::resource_link<T>>{};
			out.reserve(size(ids));

			const auto lock = std::scoped_lock{ _links_mut };

			const auto func = [this, from, getter](const unique_id id) {
				return _make_resource_link<T>(id, from, getter);
			};

			std::transform(begin(ids), end(ids), back_inserter(out), func);
			return out;
		}

		template<Resource T>
		T *data_manager::get(unique_id id, std::optional<unique_id> mod)
		{
			auto res = get<T>(id, no_load, mod);

			if (!res->loaded)
				res->load(*this);

			return res;
		}

		template<Resource T>
		T *data_manager::get(unique_id id, const no_load_t, std::optional<unique_id> mod)
		{
			auto res = get_resource(id, mod);

			try
			{
				return &dynamic_cast<T&>(*res);
			}
			catch (const std::bad_cast&)
			{
				using namespace std::string_literals;
				throw resource_wrong_type{ "Tried to get resource using wrong type, unique_id was: "s
					+ get_as_string(id) + ", requested type was: "s + typeid(T).name() + ", resource type was: " + typeid(res).name() };
			}
		}

		template<Resource T>
        inline data_manager::try_get_return<T> data_manager::try_get(unique_id id, std::optional<unique_id> mod) noexcept
		{
			auto res = try_get<T>(id, no_load, mod);

			if (res.result)
			{
				assert(res.error == get_error::ok);
				auto &r = res.result;
				if (!r->loaded)
					r->load(*this);
			}

			return res;
		}

		template<Resource T>
		inline data_manager::try_get_return<T> data_manager::try_get(unique_id id, const no_load_t, std::optional<unique_id> mod) noexcept
		{
			const auto lock = std::shared_lock{ _mut };
			return  _try_get<std::decay_t<T>>(id, no_load, mod);
		}

		// TODO: duplicate of above to support const
		template<Resource T>
		inline data_manager::try_get_return<const T> data_manager::try_get(unique_id id, const no_load_t, std::optional<unique_id> mod) const noexcept
		{
			const auto lock = std::shared_lock{ _mut };
			return  _try_get<std::decay_t<T>>(id, no_load, mod);
		}

		template<Resource T>
		inline data_manager::try_get_return<const T> data_manager::try_get_previous(const T* r) const noexcept
		{
			const auto lock = std::shared_lock{ _mut };
			
			// find the current mod
			const auto iter = std::ranges::find_if(_mod_stack, [m = r->mod](auto&& mod) {
				return mod.mod_info.id == m;
				});

			assert(iter != end(_mod_stack));
			if (iter == begin(_mod_stack))
				return { nullptr, get_error::no_resource_for_id };

			const auto prev_mod = std::prev(iter);

			return try_get<const T>(r->id, no_load, prev_mod->mod_info.id);
		}

		template<Resource T>
		resources::resource_link<T> data_manager::_make_resource_link(unique_id id, unique_id from,
			typename resources::resource_link_type<T>::get_func get)
		{
			// NOTE: expects _links_mut to be held
			using namespace std::string_literals;
			if (!id)
			{
				LOGERROR("Tried to make link to zero id from: "s + get_as_string(from));
				return {};
			}

			for (const auto& link : _resource_links)
			{
				if (link->id() == id)
				{
					const auto link_ptr = dynamic_cast<resources::resource_link_type<T>*>(link.get());
					if (link_ptr)
					{
						link->add_reverse_link(from);
						return { link_ptr };
					}
				}
			}

			auto link_type = std::make_unique<resources::resource_link_type<T>>(id, get);
			const auto ret = resources::resource_link<T>{ link_type.get() };
			link_type->add_reverse_link(from);
			_resource_links.emplace_back(std::move(link_type));
			return ret;
		}

		template<Resource T>
        inline data_manager::try_get_return<T> data_manager::_try_get(unique_id id, const no_load_t, std::optional<unique_id> mod) noexcept
		{
			auto res = _try_get_resource(id, mod);
			if (!res)
				return { nullptr, get_error::no_resource_for_id };

			auto out = dynamic_cast<T*>(res);

			if (!out)
				return { nullptr, get_error::resource_wrong_type };

			return { out, get_error::ok };
		}

		// TODO: this function is duplicated so that it can be accessed with const(no_load_t is implicit in a const context).
		template<Resource T>
		inline data_manager::try_get_return<const T> data_manager::_try_get(unique_id id, const no_load_t, std::optional<unique_id> mod) const noexcept
		{
			auto res = _try_get_resource(id, mod);
			if (!res)
				return { nullptr, get_error::no_resource_for_id };

			auto out = dynamic_cast<const T*>(res);

			if (!out)
				return { nullptr, get_error::resource_wrong_type };

			return { out, get_error::ok };
		}

		Resource auto* data_manager::_set(Resource auto res, std::string_view group)
		{
			//store the id used within the resource for ease of use
			assert(res.id != unique_zero);

			auto& m = _get_mod(res.mod);

			// add to resource storage
			const auto res_ptr = _resources.set(std::move(res));

			// add to mod
			m.resources.emplace_back(res_ptr);

			// add to resource group
			resource_group* res_group = nullptr;
			for (auto& r : m.resources_by_type)
			{
				if (r.first == group)
					res_group = &r;
			}

			if (!res_group)
				res_group = &m.resources_by_type.emplace_back(group, std::vector<const resources::resource_base*>{});

			res_group->second.emplace_back(res_ptr);

			return res_ptr;
		}

		template<Resource T>
		const T* get(unique_id id, std::optional<unique_id> mod)
		{
			return detail::get_data_manager().get<T>(id, mod);
		}

		template<Resource T>
		const T* get(const unique_id id, const no_load_t, std::optional<unique_id> mod)
		{
			return detail::get_data_manager().get<T>(id, no_load, mod);
		}

		template<Resource T>
		data_manager::try_get_return<const T> try_get(unique_id id, std::optional<unique_id> mod)
		{
			// NOTE: need to convert try_get_return<T> !not const
			//		to try_get_return<const T>
			auto [res, err] = detail::get_data_manager().try_get<T>(id, mod);
			return { res, err };
		}
	}
}
