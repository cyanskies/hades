#include "hades/data.hpp"

#include <typeindex>

#include "hades/logging.hpp"
#include "hades/resource_base.hpp"

namespace hades
{
	namespace data
	{
		template<class T>
		T* data_manager::find_or_create(unique_id target, unique_id mod)
		{
			using Type = std::remove_const_t<T>;

			static_assert(is_resource_v<Type>, "Data subsystem only works with hades resources");

			Type* r = nullptr;

			if (target == unique_zero)
			{
				LOGERROR("Tried to create resource with hades::EmptyId, this id is reserved for unset Unique Id's, resource type was: " + types::string(typeid(T).name()));
				return r;
			}

			if (!exists(target))
			{
				auto new_ptr = std::make_unique<Type>();
				r = &*new_ptr;
				set<Type>(target, std::move(new_ptr));

				r->id = target;
			}
			else
			{
				try
				{
					r = get<Type>(target, no_load);
				}
				catch (data::resource_wrong_type &e)
				{
					//name is already used for something else, this cannnot be loaded
					auto modname = get_as_string(mod);
					LOGERROR(types::string(e.what()) + "In mod: " + modname + ", name has already been used for a different resource type.");
				}
			}

			// NOTE: member variable ptr
			// this is a work around for syntax error when T == resources::mod
			// need to disambiguate r->mod when mod is also the name of T
			// compiler mistakes mod for an illegal constructor call
			auto member_ptr = &resources::resource_base::mod;
			if (r)
				r->*member_ptr = mod;

			return r;
		}

		template<typename T>
		inline std::vector<T*> data_manager::find_or_create(const std::vector<unique_id>& target, unique_id mod)
		{
			std::vector<T*> out;
			out.reserve(target.size());

			std::transform(std::begin(target), std::end(target), std::back_inserter(out), [mod, &out, this](const unique_id id) {
				return find_or_create<T>(id, mod);
			});

			return out;
		}

		template<class T>
		T *data_manager::get(unique_id id)
		{
			auto res = get<T>(id, no_load);

			if (!res->loaded)
				res->load(*this);

			return res;
		}

		template<class T>
		T *data_manager::get(unique_id id, const no_load_t)
		{
			auto res = get_resource(id);

			try
			{
				return &dynamic_cast<T&>(*res);
			}
			catch (const std::bad_cast&)
			{
				using namespace std::string_literals;
				throw resource_wrong_type{ "Tried to get resource using wrong type, unique_id was: "s
					+ get_as_string(id) + ", requested type was: "s + typeid(T).name() };
			}
		}

		template<class T>
        inline data_manager::try_get_return<T> data_manager::try_get(unique_id id) noexcept
		{
			auto res = try_get<T>(id, no_load);

			if (res.result)
			{
				assert(res.error == get_error::ok);
				auto &r = res.result;
				if (!r->loaded)
					r->load(*this);
			}

			return res;
		}

		template<class T>
        inline data_manager::try_get_return<T> data_manager::try_get(unique_id id, const no_load_t) noexcept
		{
			auto res = try_get_resource(id);
			if (!res)
				return { nullptr, get_error::no_resource_for_id };

			auto out = dynamic_cast<T*>(res);

			if (!out)
				return { nullptr, get_error::resource_wrong_type };

			return { out, get_error::ok };
		}

		template<class T>
		void data_manager::set(unique_id id, std::unique_ptr<T> ptr)
		{
			//throw error if that id has already been used
			if (exists(id))
			{
				throw resource_name_already_used("Tried to set a new resource using the already reserved name: " + get_as_string(id)
					+ ", resource type was: " + typeid(T).name());
			}

			//store the id used within the resource for ease of use
			ptr->id = id;
			_resources.insert({ id, std::move(ptr) });
		}

		template<class T>
		const T* get(unique_id id)
		{
			data_manager* data = nullptr;
			detail::exclusive_lock lock;
			std::tie(data, lock) = detail::get_data_manager_exclusive_lock();
			return data->get<T>(id);
		}

		template<class T>
		data_manager::try_get_return<const T> try_get(unique_id id)
		{
			const auto &[data, lock] = detail::get_data_manager_exclusive_lock();
			auto[anim, error] = data->try_get<T>(id);
			return { anim, error };
		}

		template<typename T>
		std::vector<unique_id> get_uid(const std::vector<const T*> &r_list)
		{
			static_assert(is_resource_v<T>, "data subsystem only works on hades resources");

			std::vector<unique_id> out;
			out.reserve(r_list.size());

			for (const auto &r : r_list)
				out.emplace_back(r->id);

			return out;
		}

	}
}
