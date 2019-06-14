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
			guard_type(const TransactionalGuard<Ty>* g) noexcept
				: c_ref{ g }
			{}

			guard_type(const guard_type& g) noexcept(std::is_nothrow_copy_constructible_v<Ty>)
				: c_ref{ g.c_ref }, ref{ g.ref }, starting_value{ m.starting_value },
					new_value{ m.new_value }
			{
				assert(!m.lock_token);
			}

			guard_type(guard_type&&) noexcept(std::is_nothrow_move_constructible_v<Ty>) = default;

			const TransactionalGuard<Ty>* c_ref = nullptr;
			TransactionalGuard<Ty>* ref = nullptr;
			Ty starting_value;
			Ty new_value;
			typename TransactionalGuard<Ty>::exchange_token lock_token;
		};

		template<typename Ty, template<typename> typename Guard>
		inline guard_type<Ty, Guard> *find_guard(const Guard<Ty> &g, std::vector<transaction::commit_entry> &v)
		{
			using guard_t = guard_type<Ty, Guard>;

			for (const auto& c : v)
			{
				guard_t *val = std::any_cast<guard_t>(&val.data);
				if (val && val->c_ref == &g)
					return val;
			}

			return nullptr;
		}

		template<typename Ty, template<typename> typename Guard>
		inline const auto* find_guard(const Guard<Ty>& g, const std::vector<transaction::commit_entry>& v)
		{
			//find guard never writes to vect, so it's safe to const_cast it
			std::vector<transaction::commit_entry>& vect = const_cast<std::vector<transaction::commit_entry>>(v);
			return find_guard(g, vect);
		}

		template<typename Ty, template<typename> typename Guard>
		inline bool get_lock(std::any &a)
		{
			using guard = guard_type<Ty, Guard>;
			guard &g = std::any_cast<guard&>(a);

			auto [success, lock] = transactional::exchange_lock(g.starting_value, *g.c_ref);

			g.lock_token = std::move(lock);
			return success;
		}

		template<typename Ty, template<typename> typename Guard>
		inline void release_lock(std::any& a)
		{
			using guard = guard_type<Ty, Guard>;
			guard& g = std::any_cast<guard&>(a);
			transactional::exchange_release(*g.c_ref, std::move(g.lock_token));
			g.ref->g.lock_token = Guard<Ty>::exchange_token{};
			return;
		}

		template<typename Ty, template<typename> typename Guard>
		inline void resolve_lock(std::any& a)
		{
			static_assert(std::is_nothrow_move_constructible_v<Ty> &&
				std::is_nothrow_move_constructible_v<Ty>);

			using guard = guard_type<Ty, Guard>;
			guard& g = std::any_cast<guard&>(a);
			//NOTE: g.ref is only set if transaction::set has been called
			// if it is still nullptr then new_value == starting_value
			// we can just skip
			if (g.ref)
				transactional::exchange_resolve(*g.ref, std::move(g.new_value), std::move(g.lock_token));
			
			g.lock_token = Guard<Ty>::exchange_token{};
			return;
		}
	}

	template<typename Ty, template<typename> typename TransactionalGuard>
	inline Ty transaction::get(const TransactionalGuard<Ty>& guard)
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
			std::any{std::move(entry)}
		};

		_data.emplace_back(std::move(c));
		return out;
	}

	template<typename Ty, template<typename> typename TransactionalGuard>
	inline Ty transaction::peek(const TransactionalGuard<Ty>& guard) const
	{
		static_assert(is_transactional_v<TransactionalGuard<Ty>>);
		using guard_type = guard_detail::guard_type<Ty, TransactionalGuard>;

		const auto entry = guard_detail::find_guard(guard, _data);
		return entry->new_value;
	}

	template<typename Ty, template<typename> typename TransactionalGuard>
	inline void transaction::set(TransactionalGuard<Ty>& guard, Ty &&value)
	{
		static_assert(is_transactional_v<TransactionalGuard<Ty>>);
		using guard_type = guard_detail::guard_type<Ty, TransactionalGuard>;

		auto g = guard_detail::find_guard(guard, _data);
		//it's an error to set a guard than hasn't been stored yet
		assert(g);
		assert(g->c_ref == &guard);
		g->ref = &guard;
		g->new_value = std::forward<Ty>(value);
		return;
	}

	namespace map_detail
	{
		template<typename Key, typename Ty, template<typename, typename> typename Map>
		struct map_type
		{
			map_type(Key k, const Map<Key, Ty>* m) noexcept(std::is_nothrow_copy_constructible_v<Key>)
				: key{ k }, c_ref{ m }
			{}

			map_type(const map_type& m) noexcept(std::is_nothrow_copy_constructible_v<Key>
				&& std::is_nothrow_copy_constructible_v<Ty>)
				: key{ m.key }, c_ref{ m.c_ref }, ref{ m.ref },
				starting_value{ m.starting_value }, new_value{ m.new_value }
			{
				assert(!m.lock_token);
			}

			map_type(map_type&&) noexcept(std::is_nothrow_move_constructible_v<Key>
				&& std::is_nothrow_move_constructible_v<Ty>) = default;


			Key key;
			const Map<Key, Ty> *c_ref = nullptr;
			Map<Key, Ty> *ref = nullptr;
			Ty starting_value;
			Ty new_value;
			typename Map<Key, Ty>::exchange_token lock_token;
		};

		template<typename Key, typename Ty, template<typename, typename> typename Map>
		inline map_type<Key, Ty, Map>* find_map(const Key& k, const Map<Key, Ty>& m, std::vector<transaction::commit_entry> &v)
		{
			using map_t = map_type<Key, Ty, Map>;
			const Map<Key, Ty>* c_m = &m;

			for (auto &elm : v)
			{
				map_t* val = std::any_cast<map_t>(&elm.data);
				if (val &&
					std::tie(val->c_ref, val->key) == std::tie(c_m, k))
				{
					return val;
				}
			}

			return nullptr;
		}

		template<typename Key, typename Ty, template<typename, typename> typename Map>
		inline const auto* find_map(const Key& k, const Map<Key, Ty>& m, const std::vector<transaction::commit_entry>& v)
		{
			std::vector<transaction::commit_entry>& vect = const_cast<std::vector<transaction::commit_entry>&>(v);
			return find_map(k, m, vect);
		}
	}

	template<typename Key, typename Ty, template<typename, typename> typename TransactionalMap>
	Ty transaction::get(const Key& key, const TransactionalMap<Key, Ty>& map)
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
				entry.starting_value, *entry.c_ref);
			entry.lock_token = std::move(lock);
			return success;
		};

		const auto release_lock = [](std::any & a)->void {
			entry_t& entry = std::any_cast<entry_t&>(a);
			transactional::exchange_release(entry.key,
				*entry.c_ref, std::move(entry.lock_token));
			entry.lock_token = decltype(entry.lock_token){};
		};

		const auto resolve_lock = [](std::any & a)->void {
			entry_t& entry = std::any_cast<entry_t&>(a);
			if(entry.ref)
				transactional::exchange_resolve(entry.key, *entry.ref, 
					std::move(entry.new_value), std::move(entry.lock_token));
			entry.lock_token = decltype(entry.lock_token){};
		};

		commit_entry c{
			get_lock,
			release_lock, 
			resolve_lock,
			std::any{std::move(entry)}
		};

		_data.emplace_back(std::move(c));

		return out;
	}

	template<typename Key, typename Ty, template<typename, typename> typename TransactionalMap>
	inline Ty transaction::peek(const Key& key, const TransactionalMap<Key, Ty>& map) const
	{
		const auto map_entry = map_detail::find_map(key, map, _data);
		assert(map_entry);
		return map_entry->new_value;
	}

	template<typename Key, typename Ty, template<typename, typename> typename TransactionalMap>
	inline void hades::transaction::set(TransactionalMap<Key, Ty> &map, const Key &key, Ty&& value)
	{
		auto entry = map_detail::find_map(key, map, _data);
		assert(entry);
		assert(entry->c_ref == &map);
		entry->ref = &map;
		entry->new_value = std::forward<Ty>(value);
	}

	namespace any_detail
	{
		template< typename Ty, typename Key, template<typename> typename Map>
		struct map_type
		{
			map_type(Key k, const Map<Key>* m) noexcept(std::is_nothrow_copy_constructible_v<Key>)
				: key{ k }, c_ref{ m }
			{}

			map_type(const map_type &m) noexcept(std::is_nothrow_copy_constructible_v<Key>
				&& std::is_nothrow_copy_constructible_v<Ty>)
				: key{ m.key }, c_ref{ m.c_ref }, ref{ m.ref },
				starting_value{ m.starting_value }, new_value{ m.new_value }
			{
				assert(!m.lock_token);
			}

			map_type(map_type&&) noexcept(std::is_nothrow_move_constructible_v<Key> 
				&& std::is_nothrow_move_constructible_v<Ty>) = default;

			Key key;
			const Map<Key>* c_ref = nullptr;
			Map<Key>* ref = nullptr;
			Ty starting_value;
			Ty new_value;
			typename Map<Key>::exchange_token lock_token;
		};

		template<typename Ty, typename Key, template<typename> typename Map>
		inline map_type<std::decay_t<Ty>, Key, Map>* find_map(const Key& k, const Map<Key>& m, std::vector<transaction::commit_entry>& v)
		{
			using map_t = map_type<std::decay_t<Ty>, Key, Map>;
			const Map<Key> *c_m = &m;

			for (auto& elm : v)
			{
				map_t* val = std::any_cast<map_t>(&elm.data);
				if (val &&
					std::tie(val->c_ref, val->key) == std::tie(c_m, k))
				{
					return val;
				}
			}

			return nullptr;
		}

		template<typename Ty, typename Key, template<typename> typename Map>
		inline const auto *find_map(const Key& k, const Map<Key>& m, const std::vector<transaction::commit_entry>& v)
		{
			std::vector<transaction::commit_entry>& vect = const_cast<std::vector<transaction::commit_entry>&>(v);
			return find_map(k, m, vect);
		}
	}

	template<typename Ty, typename Key, template<typename> typename TransactionalAnyMap>
	inline Ty hades::transaction::get(const Key& key, const TransactionalAnyMap<Key>& map)
	{
		using map_t = TransactionalAnyMap<Key>;

		static_assert(is_transactional_v<map_t>);
		assert(!any_detail::find_map<Ty>(key, map, _data));

		using entry_t = any_detail::map_type<Ty, Key, TransactionalAnyMap>;

		//make new entry
		auto entry = entry_t{ key, &map };
		const auto out = entry.new_value = entry.starting_value = transactional::get<Ty>(key, map);

		const auto get_lock = [](std::any& a)->bool {
			entry_t& entry = std::any_cast<entry_t&>(a);
			auto [success, lock] = transactional::exchange_lock(entry.key,
				entry.starting_value, *entry.c_ref);
			entry.lock_token = std::move(lock);
			return success;
		};

		const auto release_lock = [](std::any& a)->void {
			entry_t& entry = std::any_cast<entry_t&>(a);
			transactional::exchange_release(entry.key,
				*entry.c_ref, std::move(entry.lock_token));
			entry.lock_token = decltype(entry.lock_token){};
		};

		const auto resolve_lock = [](std::any& a)->void {
			entry_t& entry = std::any_cast<entry_t&>(a);
			if(entry.ref)
				transactional::exchange_resolve(entry.key, *entry.ref,
					std::move(entry.new_value), std::move(entry.lock_token));
			entry.lock_token = decltype(entry.lock_token){};
		};

		commit_entry c{
			get_lock,
			release_lock,
			resolve_lock,
			std::move(entry)
		};

		_data.emplace_back(std::move(c));

		return out;
	}

	template<typename Ty, typename Key, template<typename> typename TransactionalAnyMap>
	inline Ty transaction::peek(const Key& key, const TransactionalAnyMap<Key>& map) const
	{
		const auto entry = any_detail::find_map<Ty>(key, map, _data);
		assert(entry);
		return entry->new_value;
	}

	template<typename Ty, typename Key, template<typename> typename TransactionalAnyMap>
	inline void hades::transaction::set(TransactionalAnyMap<Key>&  map, const Key& key, Ty&& value)
	{
		auto entry = any_detail::find_map<Ty>(key, map, _data);
		assert(entry);
		assert(entry->c_ref == &map);
		entry->ref = &map;
		entry->new_value = std::forward<Ty>(value);
		return;
	}
}
