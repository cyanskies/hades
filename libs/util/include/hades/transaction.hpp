#ifndef HADES_TRANSACTION_HPP
#define HADES_TRANSACTION_HPP

#include <any>
#include <functional>
#include <vector>

// transaction allows easy use of the transactional semantics

namespace hades
{
	class transaction
	{
	public:
		struct commit_entry
		{
			using get_lock = bool(*)(std::any&);
			using release_lock = void(*)(std::any&);
			using resolve_lock = void(*)(std::any&);

			get_lock exchange_lock;
			release_lock release;
			resolve_lock commit;

			std::any data;
		};

		//get will safely get the value, caching it and the address of the guard/map 
		//in order to process the commit
		template<typename Ty, template<typename> typename TransactionalGuard>
		Ty get(const TransactionalGuard<Ty> &guard);
		template<typename Key, typename Ty, template<typename, typename> typename TransactionalMap>
		Ty get(const Key &key, const TransactionalMap<Key, Ty> &map);
		template<typename Ty, typename Key, template<typename> typename TransactionalAnyMap>
		Ty get(const Key& key, const TransactionalAnyMap<Key>& map);

		//returns the cached value or the most recently set value
		template<typename Ty, template<typename> typename TransactionalGuard>
		Ty peek(const TransactionalGuard<Ty>& guard) const;
		template<typename Key, typename Ty, template<typename, typename> typename TransactionalMap>
		Ty peek(const Key& key, const TransactionalMap<Key, Ty>& map) const;
		template<typename Ty, typename Key, template<typename> typename TransactionalAnyMap>
		Ty peek(const Key& key, const TransactionalAnyMap<Key>& map) const;

		//set will write to the new value into the cache
		template<typename Ty, template<typename> typename TransactionalGuard>
		void set(TransactionalGuard<Ty> &guard, Ty &&value);
		template<typename Key, typename Ty, template<typename, typename> typename TransactionalMap>
		void set(TransactionalMap<Key, Ty>& guard, const Key &key, Ty &&value);
		template<typename Ty, typename Key, template<typename> typename TransactionalAnyMap>
		void set(TransactionalAnyMap<Key>& guard, const Key& key, Ty&& value);

		//commit will apply all the changes as a single action
		//if it fails, then none of the values will have been writen
		bool commit();

		//abort releases all the references and clears the cache
		void abort() noexcept;

	private:
		std::vector<commit_entry> _data;
	};
}

#include "hades/detail/transaction.inl"

#endif // !HADES_UTIL_TRANSACTION_HPP
