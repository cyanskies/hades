#include "hades/shared_any_map.hpp"

#include <cassert>

namespace hades
{
	template<typename Key>
	inline shared_any_map<Key>::shared_any_map(const shared_any_map &other)
	{
		const auto lock = std::scoped_lock(other._map_mutex);

		for (auto&& elm : _map)
			if (!std::shared_lock<mutex_type>{ elm.second.mut, std::try_to_lock })
				throw shared_map_locked_elements{ "Cannot copy shared any map while some elements are locked" };

		_map = other._map;
	}

	template<typename Key>
	inline shared_any_map<Key>& shared_any_map<Key>::operator=(const shared_any_map &other)
	{
		const auto lock = std::scoped_lock(_map_mutex, other._map_mutex);

		for (auto&& elm : _map)
			if (!std::shared_lock<mutex_type>{ elm.second.mut, std::try_to_lock })
				throw shared_map_locked_elements{ "Cannot copy shared any map while some elements are locked" };

		_map = other._map;
	}

	template<typename Key>
	template<typename T>
	inline T shared_any_map<Key>::get(key_type k) const
	{
		assert(exists(k));
		const auto lock = std::shared_lock{ _map_mutex };

		try
		{
			const auto& elm = _map.at(k);
			const auto elm_lock = std::shared_lock{ elm.mut };

			return std::any_cast<T>(elm.data);
		}
		catch (const std::out_of_range& e)
		{
			throw shared_map_invalid_id{ "shared_any_map::get, tried to access unknown id: " + e.what() };
		}
		catch(const std::bad_any_cast& e)
		{
			throw shared_map_bad_type{ "tried to access shared_any_map element as the wrong type: " + e.what() };
		}
	}

	template<typename Key>
	template<typename T>
	inline shared_any_map<Key>::ptr_return_t<T> shared_any_map<Key>::get_pointer(key_type k) const
	{
		assert(exists(k));
		const auto lock = std::shared_lock{ _map_mutex };

		try
		{
			const auto& elm = _map.at(k);
			const auto elm_lock = std::shared_lock{ elm.mut };

			return { std::any_cast<T>(&elm.data), std::move(elm_lock) };
		}
		catch (const std::out_of_range& e)
		{
			throw shared_map_invalid_id{ "shared_any_map::get_pointer, tried to access unknown id: " + e.what() };
		}
	}

	template<typename Key>
	template<typename T>
	inline void shared_any_map<Key>::set(key_type id, T&& value)
	{
		assert(exists(id));
		const auto lock = std::shared_lock{ _map_mutex };

		try
		{
			auto& elm = _map.at(id);
			const auto elm_lock = std::scoped_lock{ elm.mut };
			elm.data = std::forward(value);
		}
		catch (const std::out_of_range& e)
		{
			throw shared_map_invalid_id{ "shared_any_map::set, tried to access unknown id: " + e.what() };
		}

		return;
	}

	template<typename Key>
	template<typename T>
	inline void shared_any_map<Key>::create(key_type id, T&& value)
	{
		assert(!exists(id));

		const auto lock = std::scoped_lock{ _map_mutex };

		const auto ret = _map.try_emplace(id, std::forward(value));

		if (!ret.second)
			throw shared_map_invalid_id{ "tried to create an entry using an id that was already used." };

		return;
	}

	template<typename Key>
	inline bool shared_any_map<Key>::exists(key_type id) const
	{
		const auto lock = std::shared_lock{ _map_mutex };
		return _map.find(id) != std::end(_map);
	}

	template<typename Key>
	inline void shared_any_map<Key>::erase(key_type id)
	{
		assert(exists(id));

		const auto lock = std::scoped_lock{ _map_mutex };
		const auto ret = _map.erase(id);

		assert(ret == 1);

		return;
	}

	template<typename Key>
	template<typename T>
	inline shared_any_map<Key>::lock_return shared_any_map<Key>::exchange_lock(key_type id, const T&& expected) const
	{
		assert(exists(k));
		const auto lock = std::shared_lock{ _map_mutex };

		try
		{
			const auto& elm = _map.at(k);
			auto elm_lock = std::unique_lock{ elm.mut };

			const auto ptr = std::any_cast<T>(&elm.data);

			if (!ptr)
				throw shared_map_bad_type{ "tried to access shared_any_map element as the wrong type");

			if (*ptr == expected)
				return { true, std::move(elm_lock) };
			else
			{
				elm_lock.unlock();
				return { false, std::move(elm_lock) };
			}
		}
		catch (const std::out_of_range& e)
		{
			throw shared_map_invalid_id{ "shared_any_map::get, tried to access unknown id: " + e.what() };
		}
	}

	template<typename Key>
	inline void shared_any_map<Key>::exchange_release(key_type id, exchange_token token) const noexcept
	{
		const auto valid_token = [&]()->bool {
			const auto lock = std::scoped_lock{ _map_mutex };

			const auto it = _map.find(id);

			return token.owns_lock() &&
				it != std::end(_map) &&
				token.mutex() == &it->mut;
		};

		assert(std::invoke(valid_token));
		return;
	}

	template<typename Key>
	template<typename T>
	inline void shared_any_map<Key>::exchange_resolve(key_type id, T&& desired, exchange_token token)
	{
		assert(exists(id));
		const auto lock = std::shared_lock{ _map_mutex };

		try
		{
			auto& elm = _map.at(id);
			assert(token.mutex() == &elm.mut);
			assert(token.owns_lock());
			elm.data = std::forward(desired);
		}
		catch (const std::out_of_range& e)
		{
			throw shared_map_invalid_id{ "shared_any_map::exchange_resolve, tried to access unknown id: " + e.what() };
		}

		return;
	}
}
