#include "transactional_map.hpp"

#include <cassert>
#include <exception>
#include <numeric> // for std::iota

namespace hades {
	template<typename Component, typename IdType>
	typename transactional_map<Component, IdType>::value_type transactional_map<Component, IdType>::get(key_type id) const
	{
		if (!exists(id))
			throw std::logic_error("id is not stored in this vector.");

		auto index = _getIndex(id);

		//always open a shared lock to the vectors before accessing them
		shared_lock vectlk(_vectorMutex, std::defer_lock);
		assert(index < _componentMutex.size());

		//always open an exclusive lock to the component before accessing it.
		std::unique_lock<mutex_type> complk(_componentMutex[index], std::defer_lock);
		std::lock(vectlk, complk);
		assert(index < _components.size());

		return _components[index];
	}

	//algorithm from:
	//https://stackoverflow.com/questions/17074324/how-can-i-sort-two-vectors-in-the-same-way-with-criteria-that-uses-only-one-of
	//to support the sort function
	template <typename T, typename Compare>
	std::vector<std::size_t> sort_permutation(
		const std::vector<T>& vec,
		Compare& compare)
	{
		std::vector<std::size_t> p(vec.size());
		std::iota(p.begin(), p.end(), 0);
		std::sort(p.begin(), p.end(),
			[&](std::size_t i, std::size_t j) { return compare(vec[i], vec[j]); });
		return p;
	}

	template <typename T>
	std::vector<T> apply_permutation(
		const std::vector<T>& vec,
		const std::vector<std::size_t>& p)
	{
		std::vector<T> sorted_vec(vec.size());
		std::transform(p.begin(), p.end(), sorted_vec.begin(),
			[&](std::size_t i) { return vec[i]; });
		return sorted_vec;
	}
	//end functions from stack overflow

	template<typename Component, typename IdType>
	void transactional_map<Component, IdType>::sort()
	{
		//take exclusive locks
		std::lock_guard<shared_mutex, shared_mutex> guard{ _vectorMutex, _dispatchMutex };

		//confirm that all of the mutexs are unlocked
		for (auto &&m : _componentMutex)
			if (!std::unique_lock<mutex_type>{ m, std::try_to_lock })
				throw std::logic_error("Cannot sort while any component mutexes are still being held.");

		//generate the sorting map
		std::less<key_type> less;
		auto permutationMap = sort_permutation(_ids, less);

		//apply that sorting to the other vectors(note, we don't need to sort the mutex vector)
		_ids = apply_permutation(_ids, permutationMap);
		_components = apply_permutation(_components, permutationMap);

		//store the fixed up indicies in the dispatch map
		for (size_type i = 0; i < _ids.size(); ++i)
			_idDispatch[_ids[i]] = i;
	}

	template<typename Component, typename IdType>
	bool transactional_map<Component, IdType>::exists(key_type id) const
	{
		shared_lock lk(_dispatchMutex);
		return _idDispatch.find(id) != _idDispatch.end();
	}

	template<typename Component, typename IdType>
	void transactional_map<Component, IdType>::create(key_type id, value_type value)
	{
		if (exists(id))
			throw std::logic_error("Cannot create the same component more than once.");

		//exclusive locks
		std::lock_guard<shared_mutex, shared_mutex> guard(_vectorMutex, _dispatchMutex);

		auto newpos = _components.size();
		_components.emplace_back(value);
		_ids.emplace_back(id);
		_idDispatch.insert({ id, newpos });

		//if the mutex array needs to be extended then do it.
		if (_componentMutex.size() < ++newpos)
		{
			mutex_array bigger_array(newpos);
			_componentMutex.swap(bigger_array);
		}
	}

	template<typename Component, typename IdType>
	void transactional_map<Component, IdType>::erase(key_type id)
	{
		if (!exists(id))
			throw std::logic_error("Tried to remove id that isn't contained.");

		auto index = _getIndex(id);

		assert(index < _componentMutex.size());

		//exclusive locks
		std::lock_guard<shared_mutex, shared_mutex> guard{ _vectorMutex, _dispatchMutex };

		//if index isn't the last entry, then swap it so that it's at the end
		if (index < _components.size() - 1)
		{
			//always open an exclusive lock to the component before accessing it.
			std::lock_guard<mutex_type, mutex_type> complk(_componentMutex[index], _componentMutex.back());

			//swap index and back
			std::swap(_components[index], _components.back());
			std::swap(_ids[index], _ids.back());
			_idDispatch[_ids[index]] = index;
		}

		_ids.pop_back();
		_components.pop_back();
		_componentMutex.pop_back();
		_idDispatch.erase(id);
	}

	template<typename Component, typename IdType>
	typename transactional_map<Component, IdType>::lock_return transactional_map<Component, IdType>::exchange_lock(key_type id, value_type expected) const
	{
		if (!exists(id))
			throw std::logic_error("id is not stored in this vector.");

		auto index = _getIndex(id);

		shared_lock vectlk(_vectorMutex);
		assert(index < _componentMutex.size());
		std::unique_lock<mutex_type> complk(_componentMutex[index]);

		assert(index < _components.size());
		//if the value in expected matches the store number then grant the lock
		if (_components[index] == expected)
			return std::make_tuple(true, std::move(complk));
		else
		{
			//otherwise return false and reclaim the lock
			complk.unlock();
			return std::make_tuple(false, std::move(complk));
		}
	}

	template<typename Component, typename IdType>
	void transactional_map<Component, IdType>::exchange_release(key_type id, exchange_token &&token) const
	{
		if (!exists(id))
			throw std::logic_error("id is not stored in this vector.");
		else if (!token.owns_lock())
			throw std::logic_error("token is invalid.");

		auto tok = std::move(token);

		auto index = _getIndex(id);

		shared_lock vectlk(_vectorMutex);
		assert(index < _components.size());

		if (&*tok.mutex() != &_componentMutex[index])
			throw std::logic_error("exchange token is not for this key.");
		//let the token go out of scope to unlock the mutex
	}

	template<typename Component, typename IdType>
	void transactional_map<Component, IdType>::exchange_resolve(key_type id, value_type desired, exchange_token &&token)
	{
		if (!exists(id))
			throw std::logic_error("id is not stored in this vector.");
		else if (!token.owns_lock())
			throw std::logic_error("token is invalid.");

		auto tok = std::move(token);

		auto index = _getIndex(id);

		shared_lock vectlk(_vectorMutex);
		assert(index < _components.size());

		if (&*tok.mutex() != &_componentMutex[index])
			throw std::logic_error("exchange token is not for this key.");

		_components[index] = desired;
	}

	template<typename Component, typename IdType>
	typename transactional_map<Component, IdType>::size_type transactional_map<Component, IdType>::_getIndex(IdType id) const
	{
		//index is olny useful if the arrays are all the same size, so we'll check the invarient here.
		assert((_components.size() == _ids.size()) &&
			(_ids.size() == _componentMutex.size()));
		shared_lock lk(_dispatchMutex);
		return _idDispatch.find(id)->second;
	}
}