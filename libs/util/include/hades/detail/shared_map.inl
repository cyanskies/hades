#include "hades/shared_map.hpp"

#include <cassert>
#include <exception>
#include <numeric> // for std::iota
#include <utility> // for std::forward

#include "hades/types.hpp"

namespace hades {
	template<typename Key, typename Value>
	shared_map<Key, Value>::shared_map(const shared_map &rhs) :
		_idDispatch(rhs._idDispatch), _ids(rhs._ids), _components(rhs._components),
		_componentMutex(rhs._componentMutex.size())
	{}

	template<typename Key, typename Value>
	shared_map<Key, Value>::shared_map(shared_map &&rhs) :
		_idDispatch(std::move(rhs._idDispatch)), _ids(std::move(rhs._ids)),
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

		return _components[index];
	}

	template<typename Key, typename Value>
	typename shared_map<Key, Value>::value_type shared_map<Key, Value>::get_no_async(key_type id) const
	{
		auto index = _getIndex(id);
		
		assert(index < _componentMutex.size());
		assert(index < _components.size());

		return _components[index];
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

		_components[index] = std::forward<T>(value);
		return;
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

	template<typename Key, typename Value>
	void shared_map<Key, Value>::sort()
	{
		//take exclusive locks
		const auto lock = std::scoped_lock{ _vectorMutex, _dispatchMutex };

		//confirm that all of the mutexs are unlocked
		for (auto &&m : _componentMutex)
			if (!std::unique_lock<mutex_type>{ m, std::try_to_lock })
				throw shared_map_locked_elements{ "Cannot sort shared map while any Key mutexes are still being held." };

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

	template<typename Key, typename Value>
	bool shared_map<Key, Value>::exists(key_type id) const
	{
		read_lock lk(_dispatchMutex);
		return _idDispatch.find(id) != _idDispatch.end();
	}

	/*template<class A>
	types::string as_string(A value)
	{
		return to_string(value);
	}

	template<typename A, typename B, template<typename, typename> typename C> 
	types::string as_string(C<A, B> value)
	{
		const auto &[a, b] = value;
		return to_string(a) + "::" + to_string(b);
	}*/

	template<typename Key, typename Value>
	void shared_map<Key, Value>::create(key_type id, value_type value)
	{
		//exclusive locks
		const auto lock = std::scoped_lock{ _vectorMutex, _dispatchMutex };
		
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

	template<typename Key, typename Value>
	void shared_map<Key, Value>::erase(key_type id)
	{
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
			std::swap(_ids[index], _ids.back());
			_idDispatch[_ids[index]] = index;
		}

		_ids.pop_back();
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
		if (_components[index] == expected)
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

		_components[index] = std::forward<T>(desired);
	}

	template<typename Key, typename Value>
	typename shared_map<Key, Value>::data_array shared_map<Key, Value>::data() const
	{
		const auto lock = std::scoped_lock{ _vectorMutex, _dispatchMutex };

		//confirm that all of the mutexs are unlocked
		for (auto&& m : _componentMutex)
			if (!std::unique_lock<mutex_type>{ m, std::try_to_lock })
				throw shared_map_locked_elements{ "Cannot copy data while any Key mutexes are still being held." };

		data_array output;
		output.reserve(_idDispatch.size());
		//make a copy of the data;
		for (auto key_data : _idDispatch)
			output.push_back({ key_data.first, _components[key_data.second] });

		return output;
	}

	template<typename Key, typename Value>
	typename shared_map<Key, Value>::size_type shared_map<Key, Value>::_getIndex(key_type id) const
	{
		//index is olny useful if the arrays are all the same size, so we'll check the invarient here.
		assert((_components.size() == _ids.size()) &&
			(_ids.size() == _componentMutex.size()));
		read_lock lk(_dispatchMutex);
		const auto result = _idDispatch.find(id);
		if(result == std::end(_idDispatch))
			throw shared_map_invalid_id{ "id is not stored in this shared_map." };

		return result->second;
	}
}
