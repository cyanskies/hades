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
			//LOGERROR("Tried to create resource link to incorrect type. Link from: "
				//+ get_as_string(from) + " to " + get_as_string(id));
			assert(id());
			_resource = std::invoke(_get, d, id());// data::get<T>(id());
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

			// NOTE: member variable ptr
			// this is a work around for syntax error when T == resources::mod
			// need to disambiguate r->mod when mod is also the name of T
			// compiler(MSVC) mistakes mod for an illegal constructor call
			//constexpr auto member_ptr = &resources::resource_base::mod;

			Type* r = nullptr;

			if (target == unique_zero)
			{
				LOGERROR("Tried to create resource with hades::EmptyId, this id is reserved for unset Unique Id's, resource type was: "s + string(typeid(T).name()));
				return r;
			}

			assert(!empty(_mod_stack));
			if (!mod)
				mod = _mod_stack.front().mod_info.id;

			if (!exists(target))
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
				auto [get_r, e]= try_get<Type>(target, no_load);

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
					//res.id = target;
					res.mod = *mod;
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
			assert(!empty(_mod_stack));
			if (!mod)
				mod = _mod_stack.front().mod_info.id;

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
		std::vector<resources::resource_link<T>> data_manager::make_resource_link(const std::vector<unique_id>& ids,
			const unique_id from, const typename resources::resource_link_type<T>::get_func getter)
		{
			auto out = std::vector<resources::resource_link<T>>{};
			out.reserve(size(ids));

			const auto func = [this, from, getter](const unique_id id) {
				return make_resource_link<T>(id, from, getter);
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
			auto res = try_get_resource(id, mod);
			if (!res)
				return { nullptr, get_error::no_resource_for_id };

			auto out = dynamic_cast<T*>(res);

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
			const auto res_ptr = _resources.set(res);
			//const auto res_ptr = m.resources.emplace_back(std::move(res)).get();

			// add to mod
			// don't double up on resource ptrs
			/*auto iter = std::ranges::find_if(m.resources, [id = res.id, mod = res.mod](auto&& resource) {
				return id == resource->id && mod == resource->mod;
				});
			if (iter == end(m.resources))*/
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
			data_manager* data = nullptr;
			detail::exclusive_lock lock;
			std::tie(data, lock) = detail::get_data_manager_exclusive_lock();
			return data->get<T>(id, mod);
		}

		template<Resource T>
		const T* get(const unique_id id, const no_load_t, std::optional<unique_id> mod)
		{
			auto [data, lock] = detail::get_data_manager_exclusive_lock();
			std::ignore = lock;
			return data->get<T>(id, no_load, mod);
		}

		template<Resource T>
		data_manager::try_get_return<const T> try_get(unique_id id, std::optional<unique_id> mod)
		{
			const auto &[data, lock] = detail::get_data_manager_exclusive_lock();
			auto[anim, error] = data->try_get<T>(id, mod);
			return { anim, error };
		}
	}
}
