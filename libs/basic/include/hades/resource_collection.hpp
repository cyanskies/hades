#ifndef HADES_RESOURCE_COLLECTION_HPP
#define HADES_RESOURCE_COLLECTION_HPP

#include <array>
#include <cstddef>
#include <typeindex>
#include <vector>

#include "plf_colony.h"

#include "hades/uniqueid.hpp"

namespace hades::resources
{
	struct resource_base;
}

namespace hades::data
{
	class erased_hive
	{
	public:
		template<Resource T>
		erased_hive(const T*);
		
		erased_hive(erased_hive&& rhs) noexcept
            : _move{ rhs._move }, _move_assign{ rhs._move_assign }, _destroy{ rhs._destroy }
#ifndef NDEBUG
			, _index{ rhs._index }
#endif
		{
			std::invoke(_move, rhs._mem, _mem);
			return;
		}

		erased_hive& operator=(erased_hive&& rhs) noexcept
		{
			assert(_index == rhs._index);
			std::invoke(_move_assign, rhs._mem, _mem);
			return *this;
		}

		~erased_hive() noexcept
		{
			std::invoke(_destroy, _mem);
		}

		Resource auto* set(Resource auto);
		template<Resource T>
		void erase(unique_id id, unique_id mod) noexcept;

		template<typename T>
		using hive_t = plf::colony<T>;
		// all colonies are the same size
		static_assert(sizeof(hive_t<int>) == sizeof(hive_t<std::array<int, 500>>));
		using mem_t = std::array<std::byte, sizeof(hive_t<int>)>;

	private:
		mem_t _mem;
		void(*_move)(mem_t&, mem_t&);
		void(*_move_assign)(mem_t&, mem_t&);
		void(*_destroy)(mem_t&);

#ifndef NDEBUG
		std::type_index _index;
#endif
	};

	class resource_collection
	{
	public:
		// throws if the id/mod combination already has a value
		Resource auto* set(Resource auto res);
		void erase(unique_id id, unique_id mod) noexcept
		{
			for (auto& hive : _hives)
				std::invoke(hive.erase, hive.hive, id, mod);
			return;
		}

	private:
		struct hive_storage
		{
			std::type_index type;
			void(*erase)(erased_hive&, unique_id, unique_id) noexcept;
			erased_hive hive;
		};

		template<Resource T>
		hive_storage* _get_hive();

		std::vector<hive_storage> _hives;
	};
}

#include "hades/detail/resource_collection.inl"

#endif //!HADES_RESOURCE_COLLECTION_HPP
