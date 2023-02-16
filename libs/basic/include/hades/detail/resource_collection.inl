#include "hades/resource_collection.hpp"

#include "hades/resource_base.hpp"

namespace hades::data::detail
{
	template<Resource T>
	void move_erased_hive(erased_hive::mem_t& source, erased_hive::mem_t& target) noexcept
	{
		using Hive = erased_hive::hive_t<T>;
		auto obj = std::launder(reinterpret_cast<Hive*>(&source));
		new (&target) Hive{ std::move(*obj) };
		return;
	}

	template<Resource T>
	void move_assign_erased_hive(erased_hive::mem_t& source, erased_hive::mem_t& target) noexcept
	{
		using Hive = erased_hive::hive_t<T>;
		auto src = std::launder(reinterpret_cast<Hive*>(&source));
		auto trgt = std::launder(reinterpret_cast<Hive*>(&target));
		*trgt = std::move(*src);
		return;
	}

	template<Resource T>
	void destroy_erased_hive(erased_hive::mem_t& mem) noexcept
	{
		using Hive = erased_hive::hive_t<T>;
		auto obj = std::launder(reinterpret_cast<Hive*>(&mem));
		obj->~Hive();
		return;
	}

	template<Resource T>
	bool erased_hive_elm_exists(erased_hive::hive_t<T>& hive, unique_id id, unique_id mod) noexcept
	{
		return std::ranges::any_of(hive, [id, mod](auto&& elm) {
			return elm.id == id && elm.mod == mod;
			});
	}
}

namespace hades::data
{
	template<Resource T>
	erased_hive::erased_hive(const T*)
#ifndef NDEBUG
		: _index{ typeid(T) }
#endif
	{
		new(&_mem) hive_t<T>{};
		_move = detail::move_erased_hive<T>;
		_move_assign = detail::move_assign_erased_hive<T>;
		_destroy = detail::destroy_erased_hive<T>;

		return;
	}

	Resource auto* erased_hive::set(Resource auto res)
	{
		assert(typeid(std::decay_t<decltype(res)>) == _index);
		auto hive = std::launder(reinterpret_cast<hive_t<std::decay_t<decltype(res)>>*>(&_mem));
		assert(res.id != unique_zero || res.mod != unique_zero);
		assert(!detail::erased_hive_elm_exists(*hive, res.id, res.mod));
		auto iter = hive->emplace(std::move(res));
		return &*iter;
	}

	template<Resource T>
	void erased_hive::erase(unique_id id, unique_id mod) noexcept
	{
		assert(typeid(T) == _index);
		auto hive = std::launder(reinterpret_cast<hive_t<T>*>(&_mem));
		auto beg = begin(*hive);
		auto end = std::end(*hive);
		while (beg != end)
		{
			if (beg->id == id && beg->mod == mod)
			{
				hive->erase(beg);
				break;
			}
		}
		return;
	}

	Resource auto* resource_collection::set(Resource auto res)
	{
		using Type = std::decay_t<decltype(res)>;
		auto storage = _get_hive<Type>();
		return storage->hive.set(res);
	}

	template<Resource T>
	resource_collection::hive_storage* resource_collection::_get_hive()
	{ 
		hive_storage* storage = {};
		for (auto& hive : _hives)
		{
			if (typeid(T) == hive.type)
			{
				storage = &hive;
				break;
			}
		}

		if (!storage)
		{
			const auto erase = [](erased_hive& hive, const unique_id id, const unique_id mod) noexcept{
				hive.erase<T>(id, mod);
				return;
			};

			constexpr T* ptr = {};
			storage = &_hives.emplace_back(std::type_index{ typeid(T) }, erase, erased_hive{ ptr });
		}

		return storage;
	}
}
