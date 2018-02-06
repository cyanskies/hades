#include <typeindex>

namespace hades
{
	namespace data
	{
		template<class T>
		T *data_manager::get(UniqueId id)
		{
			auto res = get<T>(id, no_load);

			if (!res->loaded)
				res->load(this);

			return res;
		}

		template<class T>
		T *data_manager::get(UniqueId id, const no_load_t)
		{
			auto res = getResource(id);
			auto out = dynamic_cast<T*>(res);

			if (!out)
			{
				throw resource_wrong_type("Tried to get resource using wrong type, unique_id was: " + getAsString(id)
					+ ", requested type was: " + typeid(T).name());
			}

			return out;
		}

		template<class T>
		void data_manager::set(UniqueId id, std::unique_ptr<T> ptr)
		{
			//throw error if that id has already been used
			if (exists(id))
			{
				throw resource_name_already_used("Tried to set a new resource using the already reserved name: " + getAsString(id)
					+ ", resource type was: " + typeid(T).name());
			}

			//store the id used within the resource for ease of use
			ptr->id = id;
			_resources.insert({ id, std::move(ptr) });
		}

		template<class T>
		const T* Get(UniqueId id)
		{
			data_manager* data = nullptr;
			detail::exclusive_lock lock;
			std::tie(data, lock) = detail::GetDataManagerPtrExclusive();
			return data->get<T>(id);
		}
	}
}