#include "Hades/DataManager.hpp"

namespace hades
{
	namespace resources
	{
		template<typename T>
		void resource_type<T>::load(data::data_manager* data)
		{
			if (_resourceLoader)
				_resourceLoader(this, data);
		}
	}

	namespace data
	{
		template<typename T>
		bool operator<(const UniqueId_t<T>& lhs, const UniqueId_t<T>& rhs)
		{
			return lhs._value < rhs._value;
		}

		template<class T>
		void data_manager::set(UniqueId key, std::unique_ptr<T> value)
		{
			try
			{
				_resources.set(key, std::move(value));
			}
			catch (type_erasure::value_wrong_type &e)
			{
				throw resource_wrong_type(e.what());
			}
		}

		template<class T>
		T* data_manager::get(UniqueId)
		{
			//exception -- value no defined
			//exception -- value is wrong type

			return nullptr;
			//auto r = Resource<T>( get_resource<T>(_resources, key) );
			//return r
		}
	}
}