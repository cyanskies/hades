#include "hades/any_map.hpp"

#include <cassert>
#include <exception>
#include <typeinfo>

namespace hades
{
	template<class Key>
	template<class T>
	T& any_map<Key>::set(const Key key, T value)
	{

		auto iter = _bag.find(key);
		if (iter == _bag.end())
		{
			auto& elm = _bag.emplace(key, std::make_any<T>(std::move(value))).first->second;
			return std::any_cast<T&>(elm);
		}

		try
		{
			auto& elm = std::any_cast<T&>(iter->second);
			elm = std::move(value);
			return elm;
		}
		catch (std::bad_any_cast&)
		{
			using namespace std::string_literals;
			throw any_map_value_wrong_type("Passed wrong type for this key to any_map, provided: "s
				+ typeid(T).name() + ", expected: "s + iter->second.type().name());
		}
	}

	template<class Key>
	template<class T>
	T& any_map<Key>::set(const allow_overwrite_t, const Key key, T value)
	{
		if (auto iter = _bag.find(key); 
			iter == _bag.end())
		{
			auto& elm = _bag.emplace(key, std::make_any<T>(std::move(value))).first->second;
			return std::any_cast<T&>(elm);
		}
		else
		{
            std::any& a = iter->second;
            return a.emplace<T>(std::move(value));
		}
	}

	namespace detail
	{
		template<typename T, typename Key, typename Map>
		inline T& any_get_impl(Key key, Map& map)
		{
			using namespace std::string_literals;
			const auto iter = map.find(key);
			if (iter == map.end())
				throw any_map_key_null("Tried to retrieve unassigned key.");

			try
			{
				return std::any_cast<T&>(iter->second);
			}
			catch (const std::bad_any_cast&)
			{
				throw any_map_value_wrong_type("Tried to retrieve value from any_map using wrong type. requested: "s
					+ typeid(T).name() + ", stored type was: "s + iter->second.type().name());
			}
		}

		template<typename T, typename Key, typename Map>
		inline T* any_try_get_impl(Key key, Map& map) noexcept
		{
			using namespace std::string_literals;
			const auto iter = map.find(key);
			if (iter == map.end())
				return nullptr;

			return std::any_cast<T>(&iter->second);
		}
	}

	template<class Key>
	template<class T>
	T any_map<Key>::get(Key key) const
	{
		return detail::any_get_impl<const T>(key, _bag);
	}

	template<class Key>
	template<class T>
	inline T& any_map<Key>::get_ref(Key key)
	{
		return detail::any_get_impl<T>(key, _bag);
	}

	template<class Key>
	template<class T>
	inline const T& any_map<Key>::get_ref(Key key) const
	{
		return detail::any_get_impl<const T>(key, _bag);
	}

	template<class Key>
	template<class T>
	inline T* any_map<Key>::try_get(Key key) noexcept
	{
		return detail::any_try_get_impl<T>(key, _bag);
	}

	template<class Key>
	template<class T>
	inline const T* any_map<Key>::try_get(Key key) const noexcept
	{
		return detail::any_try_get_impl<const T>(key, _bag);
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
	T& var_map<Key, Types...>::set(Key k, T v)
	{
		assert(std::holds_alternative<T>(_bag));
		return *_bag.insert({ k, value_type{ v } });
	}

	template<typename Key, typename ...Types>
	template<typename T>
	T var_map<Key, Types...>::get(Key k) const
	{
		assert(std::holds_alternative<T>(_bag));
		return std::get<T>(*_bag.find(k));
	}

	template<typename Key, typename ...Types>
	bool var_map<Key, Types...>::contains(Key k) const
	{
		return _bag.find(k) != std::end(_bag);
	}
}
