#ifndef HADES_SHARED_ANY_MAP_HPP
#define HADES_SHARED_ANY_MAP_HPP

#include <any>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "hades/shared_map.hpp"
#include "hades/transactional.hpp"

namespace hades 
{
	class shared_map_bad_type : public transactional_error
	{
	public:
		using transactional_error::transactional_error;
	};

	template<typename Key>
	class shared_any_map
	{
	public:
		using key_type = Key;
		using mutex_type = std::shared_mutex;
		using exchange_token = std::unique_lock<mutex_type>;
		using lock_return = std::tuple<bool, exchange_token>;

		struct data_block
		{
			template<typename T>
			explicit data_block(T&& t) : data{ std::forward<T>(t) } {}

			std::any data; 
			mutable mutex_type mut;
		};

		shared_any_map() = default;
		shared_any_map(const shared_any_map&);
		shared_any_map& operator=(const shared_any_map&);

		template<typename T>
		T get(key_type k) const;

		//gets the value without locking
		//not safe for multithreading
		template<typename T>
		T get_no_async(key_type k) const;

		template<typename T>
		using ptr_return_t = std::pair<const T*, std::shared_lock<mutex_type>>;

		template<typename T>
		ptr_return_t<T> get_pointer(key_type k) const;
		
		//sets the value without any exchange locking
		template<typename T>
		void set(key_type id, T &&value);

		//returns true if the Vector contains an entry for the id
		bool exists(key_type id) const;

		//Creates a new entry in the Vector for the id
		//slow during system update
		template<typename T>
		void create(key_type id, T &&value);
		
		//removes an entry
		//slow during system update
		//should never really be done anyway
		void erase(key_type id);
		void clear();

		//returns true if the lock was granted, the lock is only granted if expected == the current stored value
		template<typename T>
		lock_return exchange_lock(key_type id, const T &expected) const;
		//releases the lock that is already held, without commiting the changes
		void exchange_release(key_type id, exchange_token token) const noexcept;
		//releases the lock after exchanging the value
		template<typename T>
		void exchange_resolve(key_type id, T &&desired, exchange_token token);
	
	private:
		mutable mutex_type _map_mutex;
		std::unordered_map<key_type, data_block> _map;	
	};

	template<typename T>
	struct is_transactional<shared_any_map<T>> : std::true_type {};
}

#include "hades/detail/shared_any_map.inl"

#endif // !HADES_SHARED_ANY_MAP_HPP
