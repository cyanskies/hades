#include "hades/transaction.hpp"

#include <cassert>
#include <optional>
#include <tuple>

#include "hades/transactional.hpp"

namespace hades
{
	namespace guard_detail
	{
		template<typename Ty, template<typename> typename TransactionalGuard>
		struct guard_type
		{
			TransactionalGuard<Ty>* ref;
			Ty starting_value;
			Ty new_value;
			typename TransactionalGuard<Ty>::exchange_token lock_token;
		};

		template<typename Ty, template<typename> typename Guard>
		inline transaction::commit_entry *find_guard(const Guard<Ty> &g, std::vector<transaction::commit_entry> &v)
		{
			using guard_t = guard_type<Ty, Guard>;

			for (const auto& c : v)
			{
				const guard_t *val = std::any_cast<guard_t>(&val.data);
				if (val && val->ref == &g)
					return &c;
			}

			return nullptr;
		}

		template<typename Ty, template<typename> typename Guard>
		inline bool get_lock(std::any &a)
		{
			using guard = guard_type<Ty, Guard>;
			guard &g = std::any_cast<guard&>(a);

			auto [success, lock] = transactional::exchange_lock(g.starting_value, *g.ref);

			g.lock_token = std::move(lock);
			return success;
		}

		template<typename Ty, template<typename> typename Guard>
		inline void release_lock(std::any& a)
		{
			using guard = guard_type<Ty, Guard>;
			guard& g = std::any_cast<guard&>(a);
			transactional::exchange_release(*g.ref, std::move(g.lock_token));
			g.ref->g.lock_token = Guard<Ty>::exchange_token{};
		}

		template<typename Ty, template<typename> typename Guard>
		inline void resolve_lock(std::any& a)
		{
			static_assert(std::is_nothrow_move_constructible_v<Ty> &&
				std::is_nothrow_move_constructible_v<Ty>);

			using guard = guard_type<Ty, Guard>;
			guard& g = std::any_cast<guard&>(a);
			transactional::exchange_resolve(*g.ref, std::move(g.new_value), std::move(g.lock_token));
			g.ref->g.lock_token = Guard<Ty>::exchange_token{};
		}
	}

	template<typename Ty, template<typename> typename TransactionalGuard>
	inline Ty transaction::get(TransactionalGuard<Ty>& guard)
	{	
		static_assert(is_transactional_v<TransactionalGuard<Ty>>);
		using guard_type = guard_detail::guard_type<Ty, TransactionalGuard>;
		
		// its a bug to get the same guard twice
		assert(!guard_detail::find_guard(guard, _data));

		//make new entry
		guard_type entry = guard_type{ &guard };
		const auto out = entry.new_value = entry.starting_value = transactional::get(guard);
		
		commit_entry c{
			guard_detail::get_lock<Ty, TransactionalGuard>,
			guard_detail::release_lock<Ty, TransactionalGuard>,
			guard_detail::resolve_lock<Ty, TransactionalGuard>,
			std::move(entry);
		};

		_data.emplace_back(std::move(c));
		return out;
	}

	template<typename Ty, template<typename> typename TransactionalGuard>
	inline void transaction::set(TransactionalGuard<Ty>& guard, Ty &&value)
	{
		static_assert(is_transactional_v<TransactionalGuard<Ty>>);
		using guard_type = guard_detail::guard_type<Ty, TransactionalGuard>;

		auto entry = guard_detail::find_guard(guard, _data);
		//it's an error to set a guard than hasn't been stored yet
		assert(entry);

		guard_type &g = std::any_cast<guard_type&>(entry->data);
		g.new_value = std::forward(value);
	}

	namespace map_detail
	{
		template<typename Key, typename Ty, template<typename, typename> typename Map>
		struct map_type
		{
			Key key;
			Map<Key, Ty> *ref;
			Ty starting_value;
			Ty new_value;
			typename Map<Key, Ty>::exchange_token lock_token;
		};

		template<typename Key, typename Ty, template<typename, typename> typename Map>
		typename transaction::commit_entry* find_map(const Key& k, const Map<Key, Ty>& m, std::vector<transaction::commit_entry> &v)
		{
			using map_t = map_type<Key, Ty, Map>;

			for (const auto &elm : v)
			{
				const map_t* val = std::any_cast<map_t>(&elm.data);
				if (val &&
					std::tie(&val->ref, val->key) == std::tie(&m, k))
				{
					return &elm;
				}
			}

			return nullptr;
		}
	}

	template<typename Key, typename Ty, template<typename, typename> typename TransactionalMap>
	Ty transaction::get(const Key& key, TransactionalMap<Key, Ty>& map)
	{
		using map_t = TransactionalMap<Key, Ty>;

		static_assert(is_transactional_v<map_t>);
		assert(!map_detail::find_map(key, map, _data));

		using entry_t = map_detail::map_type<Key, Ty, TransactionalMap>;

		//make new entry
		auto entry = entry_t{ key, &map };
		const auto out = entry.new_value = entry.starting_value = transactional::get(key, map);

		const auto get_lock = [](std::any &a)->bool {
			entry_t& entry = std::any_cast<entry_t&>(a);
			auto [success, lock] = transactional::exchange_lock(entry.key,
				entry.starting_value, *entry.ref);
			entry.lock_token = std::move(lock);
			return success;
		};

		const auto release_lock = [](std::any & a)->void {
			entry_t& entry = std::any_cast<entry_t&>(a);
			transactional::exchange_release(entry.key,
				*entry.ref, std::move(entry.lock_token));
			entry.lock_token = decltype(entry.lock_token){};
		};

		const auto resolve_lock = [](std::any & a)->void {
			entry_t& entry = std::any_cast<entry_t&>(a);
			transactional::exchange_resolve(entry.key, *entry.ref, 
				std::move(entry.new_value), std::move(entry.lock_token));
			entry.lock_token = decltype(entry.lock_token){};
		};

		commit_entry c{
			get_lock,
			release_lock, 
			resolve_lock,
			std::move(entry);
		};

		_data.emplace_back(std::move(c));

		return out;
	}

	template<typename Key, typename Ty, template<typename, typename> typename TransactionalMap>
	inline void hades::transaction::set(TransactionalMap<Key, Ty> &map, const Key &key, Ty&& value)
	{
		commit_entry *map_entry = map_detail::find_map(key, map, _data);
		assert(map_entry);

		using entry_t = map_detail::map_type<Key, Ty, TransactionalMap>;

		entry_t &entry = std::any_cast<entry_t&>(map_entry->data);
		entry.new_value = std::forward(value);
	}
}
