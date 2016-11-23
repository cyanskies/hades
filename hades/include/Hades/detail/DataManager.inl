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
			set_resource(_resources, key, value);
		}

		template<class T>
		Resource<T> data_manager::get(UniqueId key) const
		{
			return Resource<T>();
			//auto r = Resource<T>( get_resource<T>(_resources, key) );
			//return r
		}
	}
}