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

		template<typename Ty, template<typename> typename TransactionalGuard>
		Ty get(TransactionalGuard<Ty> &guard);

		template<typename Key, typename Ty, template<typename, typename> typename TransactionalMap>
		Ty get(const Key &key, TransactionalMap<Key, Ty> &map);

		template<typename Ty, typename Key, template<typename> typename TransactionalAnyMap>
		Ty get(const Key& key, TransactionalAnyMap<Key>& map);

		template<typename Ty, template<typename> typename TransactionalGuard>
		void set(TransactionalGuard<Ty> &guard, Ty &&value);

		template<typename Key, typename Ty, template<typename, typename> typename TransactionalMap>
		void set(TransactionalMap<Key, Ty>& guard, const Key &key, Ty &&value);
		
		template<typename Ty, typename Key, template<typename> typename TransactionalAnyMap>
		void set(TransactionalAnyMap<Key>& guard, const Key& key, Ty&& value);

		bool commit();
		void abort() noexcept;

	private:
		std::vector<commit_entry> _data;
	};
}

#include "hades/detail/transaction.inl"

#endif // !HADES_UTIL_TRANSACTION_HPP
