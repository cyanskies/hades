#ifndef HADES_TRANSACTIONALMAP_HPP
#define HADES_TRANSACTIONALMAP_HPP

#include <cstddef> //for size_t
#include <map>
#include <mutex>
#include <shared_mutex>
#include <tuple>
#include <vector>

//a thread safe storage system for entity components
//stores a vector of components, once entry for each entity that possesses that component type

//usage

//get the component with auto c = comps.get(id);
//run operations
//then call exchangeLock on this an any other components you have(even data from value_guard, shared_guard)
// if(exchangeLock(id, oldc))
//then call exchange resolve on all locked components

//for each component, exchangeResolve(id, newc);

namespace hades {

	//TODO: proper exceptions
	// currenty throws runtime error if id requested is missing
	// or locks are being held incorrectly

	template<typename Key, typename Value>
	class shared_map
	{
	public:
		using value_type = Value;
		using key_type = Key;
		using mutex_type = std::shared_mutex;
		using exchange_token = std::unique_lock<mutex_type>;
		using lock_return = std::tuple<bool, exchange_token>;

		shared_map() = default;
		shared_map(const shared_map&);
		shared_map(shared_map&&);

		//returns a copy of the component for id
		value_type get(key_type id) const;

		//sets the value without any locking
		void set(key_type id, value_type value);

		//sorts the data by id for faster access
		//never call during system update
		void sort();

		//returns true if the Vector contains an entry for the id
		bool exists(key_type id) const;

		//Creates a new entry in the Vector for the id
		//slow during system update
		void create(key_type id, value_type value);
		//removes an entry
		//slow during system update
		//should never really be done anyway
		void erase(key_type id);

		//returns true if the lock was granted, the lock is only granted if expected == the current stored value
		lock_return exchange_lock(key_type id, value_type expected) const;
		//releases the lock that is already held, without commiting the changes
		void exchange_release(key_type id, exchange_token &&token) const;
		//releases the lock after exchanging the value
		void exchange_resolve(key_type id, value_type desired, exchange_token &&token);

		//get_data
		//return vector of id's to values
		using data_array = std::vector<std::pair<key_type, value_type>>;
		data_array data() const;
	private:

		using size_type = std::size_t;
		using dispatch_map = std::map<key_type, size_type>;
		using component_array = std::vector<value_type>;
		using id_array = std::vector<key_type>;
		using mutex_array = std::vector<mutex_type>;
		using exclusive_lock = std::unique_lock<mutex_type>;
		using read_lock = std::shared_lock<mutex_type>;
		using write_lock = std::lock_guard<mutex_type>;

		size_type _getIndex(key_type id) const;

		//guards access to the two vectors
		//must be taken in exclusive mode to modify the size of the vectors(id_array, _components and _componentMutex)
		mutable mutex_type _vectorMutex;
		//guards access to the dispatch map // must be taken in exclusive mode to add or remove entries
		mutable mutex_type _dispatchMutex;

		//a map to connect ids to array addresses
		dispatch_map _idDispatch;

		//these arrays must all remain sorted together, the array id in one corrisponds to the same data in the others
		//array of id's, so that we can find id's from array indexs
		id_array _ids;
		//array of mutexs to guard access to the Component array entries
		mutable mutex_array _componentMutex;
		//array of components, the main data held by this structure
		component_array _components;
	};
}

#include "hades/detail/shared_map.inl"

#endif // !HADES_UTIL_TRANSACTIONALMAP_HPP
