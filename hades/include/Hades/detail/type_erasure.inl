#include "Hades/type_erasure.hpp"

#include <exception>

namespace hades
{
	template<class Key, class type_base>
	template<class T, template<typename> class type_holder>
	void property_bag<Key, type_base>::set(Key key, const T &value)
	{
		/*static_assert(type_holder<T>::type_base == type_base);
		static_assert(std::is_base_of<type_base, type_holder>);
		static_assert(std::is_base_of<type_erased_base, type_base>);
		static_assert(std::is_base_of<type_erased, type_holder>);*/

		auto iter = _bag.find(key);

		if (iter == _bag.end())
		{
			//if the key hasn't been assigned yet, then make it
			auto ptr = std::make_unique<type_holder<T>>();
			ptr->set(value);
			_bag[key] = std::move(ptr);
		}
		else
		{
			//identifier already exsists, ditermine if it is the same type as T, then assign it.
			if (type_holder<T>::get_type_static() == iter->second->get_type())
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
	template<class T, template<typename> class type_holder>
	T property_bag<Key, type_base>::get(Key key) const
	{
		//static_assert(typename type_holder<T>::type_base == type_base);
		/*static_assert(std::is_base_of<type_base, type_holder<T>>, "");
		static_assert(std::is_base_of<type_erased_base, type_base>);
		static_assert(std::is_base_of<type_erased, type_holder>);*/

		auto iter = _bag.find(key);
		if (iter == _bag.end())
			throw std::logic_error("Tried to retrieve unassigned key.");

		if (type_holder<T>::get_type_static() == iter->second->get_type())
		{
			auto *holder = static_cast<type_holder<T>*>(&*iter->second);
			return holder->get();
		}
		else
		{
			throw std::logic_error("Tried to retrieve value from property_bag using wrong type.");
		}
	}

	template<class Key, class type_base>
	template<class T, template<typename> class type_holder>
	typename property_bag<Key, type_base>::reference property_bag<Key, type_base>::get_reference(Key key)
	{
		auto iter = _bag.find(key);
		if (iter == _bag.end())
			throw std::logic_error("Tried to retrieve unassigned key.");

		if (type_holder<T>::get_type_static() == iter->second->get_type())
		{
			auto *holder = static_cast<type_holder<T>*>(&*iter->second);
			return holder->get();
		}
		else
		{
			throw std::logic_error("Tried to retrieve value from property_bag using wrong type.");
		}
	}

	template<class Key, class type_base>
	typename property_bag<Key, type_base>::iterator property_bag<Key, type_base>::begin()
	{
		return _bag.begin();
	}

	template<class Key, class type_base>
	typename property_bag<Key, type_base>::iterator property_bag<Key, type_base>::end()
	{
		return _bag.end();
	}

	template<class Key, class type_base>
	bool  property_bag<Key, type_base>::empty()
	{
		return _bag.empty();
	}
}