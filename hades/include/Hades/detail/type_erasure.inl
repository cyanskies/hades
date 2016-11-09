#include "Hades/type_erasure.hpp"

#include <exception>

namespace hades
{
	template<class Key, class type_base>
	template<class T, class type_holder>
	void property_bag<Key, type_base>::set(Key key, T &value)
	{
		static_assert(type_holder<T>::type_base == type_base);
		static_assert(std::is_base_of<type_base, type_holder>);
		static_assert(std::is_base_of<type_erased_base, type_base>);
		static_assert(std::is_base_of<type_erased, type_holder>);

		auto iter = _bag.find(key);

		if (iter == _bag.end())
		{
			//if the key hasn't been assigned yet, then make it
			std::unique_ptr< type_holder<T> > ptr(new type_holder<T>(value));
			_bag[identifier] = std::move(ptr);
		}
		else
		{
			//identifier already exsists, ditermine if it is the same type as T, then assign it.
			if (type_holder<T>::type_static() == iter->second->type())
			{
				auto currentVal = static_cast< type_holder<T>* >(&*iter->second)->get();
				static_cast< type_holder<T>* >(&*iter->second)->set(value);
			}
			else
			{
				throw std::logic_error("Passed wrong type for this key to property_bag.");
			}
		}
	}

	template<class Key, class type_base>
	template<class T, class type_holder>
	T const &property_bag<Key, type_base>::get(Key key) const
	{
		static_assert(type_holder<T>::type_base == type_base);
		static_assert(std::is_base_of<type_base, type_holder>);
		static_assert(std::is_base_of<type_erased_base, type_base>);
		static_assert(std::is_base_of<type_erased, type_holder>);

		auto iter = _bag.find(identifier);
		if (iter == _bag.end())
			throw std::logic_error("Tried to retrieve unassigned key.");

		if (type_holder<T>::type_static() == iter->second->type())
			return static_cast< type_holder<T>* >(&*iter->second)->get();
		else
		{
			throw std::logic_error("Tried to retrieve value from property_bag using wrong type.");
		}
	}
}