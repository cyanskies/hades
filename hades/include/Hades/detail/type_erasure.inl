#include <cassert>
#include <exception>

namespace hades
{
	template<class Key>
	template<class T>
	void property_bag<Key>::set(Key key, T value)
	{
		/*static_assert(type_holder<T>::type_base == type_base);
		static_assert(std::is_base_of<type_base, type_holder>);
		static_assert(std::is_base_of<type_erased_base, type_base>);
		static_assert(std::is_base_of<type_erased, type_holder>);*/

		using type_holder = type_erasure::type_erased<T>;

		auto iter = _bag.find(key);

		if (iter == _bag.end())
		{
			//if the key hasn't been assigned yet, then make it);
			std::unique_ptr<base_type> ptr(new type_holder() );
			auto e = _bag.emplace(std::make_pair(key, std::move(ptr)));
			if (!e.second)
				throw std::runtime_error("emplace into property_bag failed");

			iter = e.first;
		}

		//identifier already exsists(or has been created, ditermine if it is the same type as T, then assign it.
		if (type_holder::get_type_static() == iter->second->get_type())
		{		
			assert(dynamic_cast<type_holder*>(&*iter->second));
			type_holder* currentVal = static_cast< type_holder* >(&*iter->second);
			static_cast< type_holder* >(&*iter->second)->set(std::move(value));
		}
		else
		{
			throw type_erasure::value_wrong_type((std::string("Passed wrong type for this key to property_bag, expected: ") + typeid(T).name()).c_str());
		}
	}

	template<class Key>
	template<class T>
	T property_bag<Key>::get(Key key) const
	{
		//static_assert(typename type_holder<T>::type_base == type_base);
		/*static_assert(std::is_base_of<type_base, type_holder<T>>, "");
		static_assert(std::is_base_of<type_erased_base, type_base>);
		static_assert(std::is_base_of<type_erased, type_holder>);*/

		using type_holder = type_erasure::type_erased<T>;

		auto iter = _bag.find(key);
		if (iter == _bag.end())
			throw type_erasure::key_null("Tried to retrieve unassigned key.");

		if (type_holder::get_type_static() == iter->second->get_type())
		{
			assert(dynamic_cast<type_holder*>(&*iter->second));
			auto *holder = static_cast<type_holder*>(&*iter->second);
			return holder->get();
		}
		else
		{
			throw type_erasure::value_wrong_type("Tried to retrieve value from property_bag using wrong type.");
		}
	}

	template<class Key>
	template<class T>
	T* property_bag<Key>::get_reference(Key key)
	{
		using type_holder = type_erasure::type_erased<T>;

		auto iter = _bag.find(key);
		if (iter == _bag.end())
			throw type_erasure::key_null("Tried to retrieve unassigned key.");

		if (type_holder::get_type_static() == iter->second->get_type())
		{
			assert(dynamic_cast<type_holder*>(&*iter->second));
			auto *holder = static_cast<type_holder*>(&*iter->second);
			return &holder->get();
		}
		else
		{
			throw type_erasure::value_wrong_type("Tried to retrieve value from property_bag using wrong type.");
		}
	}

	template<class Key>
	void* property_bag<Key>::get_reference_void(Key key)
	{
		using type_holder = type_erasure::type_erased<intptr_t>; //intptr_t is the platforms ptr size

		auto iter = _bag.find(key);
		if (iter == _bag.end())
			throw type_erasure::key_null("Tried to retrieve unassigned key.");

		auto *holder = static_cast<type_holder*>(&*iter->second);
		return static_cast<void*>(&holder->get());
	}

	template<class Key>
	typename property_bag<Key>::iterator property_bag<Key>::begin() noexcept
	{
		return _bag.begin();
	}

	template<class Key>
	typename property_bag<Key>::iterator property_bag<Key>::end() noexcept
	{
		return _bag.end();
	}

	template<class Key>
	typename property_bag<Key>::const_iterator property_bag<Key>::begin() const noexcept
	{
		return cbegin();
	}

	template<class Key>
	typename property_bag<Key>::const_iterator property_bag<Key>::end() const noexcept
	{
		return cend();
	}

	template<class Key>
	typename property_bag<Key>::const_iterator property_bag<Key>::cbegin() const noexcept
	{
		return _bag.cbegin();
	}

	template<class Key>
	typename property_bag<Key>::const_iterator property_bag<Key>::cend() const noexcept
	{
		return _bag.cend();
	}

	template<class Key>
	typename property_bag<Key>::size_type property_bag<Key>::size() const noexcept
	{
		return _bag.size();
	}

	template<class Key>
	bool  property_bag<Key>::empty()
	{
		return _bag.empty();
	}
}