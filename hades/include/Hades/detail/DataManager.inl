#include "Hades/DataManager.hpp"

namespace hades
{
	namespace data
	{
		template<typename T>
		bool operator<(const UniqueId_t<T>& lhs, const UniqueId_t<T>& rhs)
		{
			return lhs._value < rhs._value;
		}

		template<class T>
		void data_manager::set(UniqueId key, T value)
		{
			_resources.set(key, value);
		}

		template<class T>
		T const *data_manager::get(UniqueId) const
		{
			return nullptr;
			//auto r = Resource<T>( get_resource<T>(_resources, key) );
			//return r
		}
	}
}