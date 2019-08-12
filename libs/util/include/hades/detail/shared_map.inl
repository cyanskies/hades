#include "hades/shared_map.hpp"

#include <cassert>
#include <exception>
#include <numeric> // for std::iota
#include <utility> // for std::forward

#include "hades/types.hpp"

namespace hades {
	template<typename Key, typename Value>
	shared_map<Key, Value>::shared_map(const shared_map &rhs) :
		_idDispatch(rhs._idDispatch), _components(rhs._components),
		_componentMutex(rhs._componentMutex.size())
	{}

	template<typename Key, typename Value>
	shared_map<Key, Value>::shared_map(shared_map &&rhs) :
		_idDispatch(std::move(rhs._idDispatch)),
		_components(std::move(rhs._components)), _componentMutex(rhs._componentMutex.size())
	{}

	template<typename Key, typename Value>
	typename shared_map<Key, Value>::value_type shared_map<Key, Value>::get(key_type id) const
	{
		auto index = _getIndex(id);

		//always open a shared lock to the vectors before accessing them
		read_lock vectlk(_vectorMutex, std::defer_lock);
		assert(index < _componentMutex.size());

		//always open an exclusive lock to the Key before accessing it.
		std::unique_lock<mutex_type> complk(_componentMutex[index], std::defer_lock);
		std::lock(vectlk, complk);
		assert(index < _components.size());

		return _components[index].second;
	}

	template<typename Key, typename Value>
	typename shared_map<Key, Value>::value_type &shared_map<Key, Value>::get_no_async(key_type id)
	{
		auto index = _get_index_noasync(id);
		
		assert(index < _componentMutex.size());
		assert(index < _components.size());

		return _components[index].second;
	}

	template<typename Key, typename Value>
	template<typename T, typename>
	inline void shared_map<Key, Value>::set(key_type id, T &&value)
	{
		auto index = _getIndex(id);

		//always open a shared lock to the vectors before accessing them
		read_lock vectlk(_vectorMutex, std::defer_lock);
		assert(index < _componentMutex.size());

		//always open an exclusive lock to the Key before accessing it.
		std::unique_lock<mutex_type> complk(_componentMutex[index], std::defer_lock);
		std::lock(vectlk, complk);
		assert(index < _components.size());

		_components[index].second = std::forward<T>(value);
		return;
	}

	template<typename Key, typename Value>
	bool shared_map<Key, Value>::exists(key_type id) const
	{
		read_lock lk(_dispatchMutex);
		return _idDispatch.find(id) != _idDispatch.end();
	}

	template<typename Key, typename Value>
	inline bool shared_map<Key, Value>::exists_no_async(key_type id) const
	{
		return _idDispatch.find(id) != _idDispatch.end();
	}

	template<typename Key, typename Value>
	void shared_map<Key, Value>::create(key_type id, value_type value)
	{
		//exclusive locks
		const auto lock = std::scoped_lock{ _vectorMutex, _dispatchMutex };
		
		auto newpos = _components.size();
		_components.emplace_back( id, value );
		_idDispatch.insert({ id, newpos });

		//if the mutex array needs to be extended then do it.
		if (_componentMutex.size() < ++newpos)
		{
			mutex_array bigger_array(newpos);
			_componentMutex.swap(bigger_array);
		}
	}

	template<typename Key, typename Value>
	void shared_map<Key, Value>::erase(key_type id)
	{
		//TODO: check this for correctness, it basically never gets called
		const auto index = _getIndex(id);

		assert(index < _componentMutex.size());

		//exclusive locks
		const auto lock = std::scoped_lock(_vectorMutex, _dispatchMutex);

		//if index isn't the last entry, then swap it so that it's at the end
		if (index < _components.size() - 1)
		{
			//always open an exclusive lock to the Key before accessing it.
			//we lock the end aswell so we can swap and erase easily
			const auto lock_comp = std::scoped_lock{ _componentMutex[index], _componentMutex.back() };

			//swap index and back
			std::swap(_components[index], _components.back());
			_idDispatch[_components[index].first] = index;
		}

		_components.pop_back();
		_componentMutex.pop_back();
		_idDispatch.erase(id);
	}

	template<typename Key, typename Value>
	typename shared_map<Key, Value>::lock_return shared_map<Key, Value>::exchange_lock(key_type id, value_type expected) const
	{
		auto index = _getIndex(id);

		std::shared_lock<mutex_type> vectlk(_vectorMutex);
		assert(index < _componentMutex.size());
		std::unique_lock<mutex_type> complk(_componentMutex[index]);

		assert(index < _components.size());
		//if the value in expected matches the store number then grant the lock
		if (_components[index].second == expected)
			return std::make_tuple(true, std::move(complk));
		else
		{
			//otherwise return false and reclaim the lock
			complk.unlock();
			return std::make_tuple(false, std::move(complk));
		}
	}

	template<typename Key, typename Value>
	void shared_map<Key, Value>::exchange_release(key_type id, exchange_token token) const noexcept
	{
		const auto valid_token = [&]()->bool {
			const auto index = _getIndex(id);
			const auto lock = read_lock{ _vectorMutex };
	
			return token.owns_lock() && 
				index < std::size(_components) &&
				token.mutex() == &_componentMutex[index];
		};

		assert(std::invoke(valid_token));
	}

	template<typename Key, typename Value>
	template<typename T, typename>
	void shared_map<Key, Value>::exchange_resolve(key_type id, T &&desired, exchange_token token)
	{
		assert(token.owns_lock());
		const auto index = _getIndex(id);
		const auto lock_vect = read_lock{ _vectorMutex };
		assert(index < _components.size());
		assert(token.mutex() == &_componentMutex[index]);

		_components[index].second = std::forward<T>(desired);
	}

	template<typename Key, typename Value>
	inline const typename shared_map<Key, Value>::data_array& shared_map<Key, Value>::data_no_async() const noexcept
	{
		return _components;
	}

	template<typename Key, typename Value>
	typename shared_map<Key, Value>::size_type shared_map<Key, Value>::_getIndex(key_type id) const
	{
		//index is olny useful if the arrays are all the same size, so we'll check the invarient here.
		assert(_components.size() == _componentMutex.size());
		read_lock lk(_dispatchMutex);
		const auto result = _idDispatch.find(id);
		if(result == std::end(_idDispatch))
			throw shared_map_invalid_id{ "id is not stored in this shared_map." };

		return result->second;
	}
	template<typename Key, typename Value>
	inline typename shared_map<Key, Value>::size_type shared_map<Key, Value>::_get_index_noasync(key_type id) const
	{
		assert(_components.size() == _componentMutex.size());
		const auto result = _idDispatch.find(id);
		if (result == std::end(_idDispatch))
			throw shared_map_invalid_id{ "id is not stored in this shared_map." };

		return result->second;
	}
}
