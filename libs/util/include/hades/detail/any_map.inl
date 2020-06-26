#include "hades/any_map.hpp"

#include <cassert>
#include <exception>
#include <typeinfo>

namespace hades
{
	template<class Key>
	template<class T>
	void any_map<Key>::set(Key key, T value)
	{
		auto iter = _bag.find(key);
		if (iter == _bag.end())
			_bag.emplace(key, std::make_any(std::move(value)));
		else
		{
			auto t = iter->type();
			if (t == typeid(T))
				*iter = std::make_any(std::move(value));
			else
				throw any_map_value_wrong_type("Passed wrong type for this key to any_map, expected: " + t.name());
		}
	}

	template<class Key>
	template<class T>
	T any_map<Key>::get(Key key) const
	{
		return get_ref(key);
	}

	template<class Key>
	template<class T>
	inline const T& any_map<Key>::get_ref(Key key) const
	{
		const auto iter = _bag.find(key);
		if (iter == _bag.end())
			throw any_map_key_null("Tried to retrieve unassigned key.");
		else if (iter->type() == typeid(T))
			return std::any_cast<const T>(*iter);
		else
			throw any_map_value_wrong_type("Tried to retrieve value from any_map using wrong type. requested: "
				+ typeid(T).name() + ", stored type was: " + iter->type().name());
	}

	template<class Key>
	template<class T>
	inline const T* any_map<Key>::try_get(Key key) const noexcept
	{
		const auto iter = _bag.find(key);
		if (iter != end(_bag) && iter->type() == typeid(T))
			return std::any_cast<const T*>(*iter);

		return nullptr;
	}

	template<class Key>
	void any_map<Key>::erase(Key key)
	{
		auto iter = _bag.find(key);
		if (iter == _bag.end())
			throw any_map_key_null("Tried to erase unassigned key.");
		else
			return _bag.erase(iter);
	}


	template<class Key>
	bool any_map<Key>::contains(Key key) const noexcept
	{
		return _bag.find(key) != std::end(_bag);
	}

	template<class Key>
	template<class T>
	bool any_map<Key>::is(Key key) const
	{
		auto iter = _bag.find(key);
		if (iter == _bag.end())
			throw any_map_key_null("Tried to examine unassigned key.");
		else
			return iter->type() == typeid(T);
	}

	template<typename Key, typename ...Types>
	template<typename T>
	void var_map<Key, Types...>::set(Key k, T v)
	{
		static_assert(std::holds_alternative<T>(_bag), "var bag doesn't contain the requested type");
		_bag.insert({ k, value_type{ v } });
	}

	template<typename Key, typename ...Types>
	template<typename T>
	T var_map<Key, Types...>::get(Key k) const
	{
		static_assert(std::holds_alternative<T>(_bag), "var bag doesn't contain the requested type");
		return std::get<T>(*_bag.find(k));
	}

	template<typename Key, typename ...Types>
	bool var_map<Key, Types...>::contains(Key k) const
	{
		return _bag.find(k) != std::end(_bag);
	}
}
