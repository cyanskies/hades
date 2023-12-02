#ifndef HADES_RESOURCE_COLLECTION_HPP
#define HADES_RESOURCE_COLLECTION_HPP

#include <map>

#include "hades/resource_base.hpp"
#include "hades/uniqueid.hpp"

namespace hades::data
{
	class resource_collection
	{
	public:
		// throws if the id/mod combination already has a value
		Resource auto* set(Resource auto res)
		{
			auto ptr = std::make_unique<decltype(res)>(std::move(res));
			auto ret = ptr.get();
			const auto [iter, good] = _res.try_emplace({ ret->id, ret->mod }, std::move(ptr));
			if (!good)
				throw data::resource_name_already_used{ "Attempted to create a resource with an id that has already been used" };
			return ret;
		}

		void erase(unique_id id, unique_id mod) noexcept
		{
			_res.erase({ id, mod });
		}

	private:
		std::map<std::pair<unique_id, unique_id>, std::unique_ptr<resources::resource_base>> _res;
	};
}

#endif //!HADES_RESOURCE_COLLECTION_HPP
