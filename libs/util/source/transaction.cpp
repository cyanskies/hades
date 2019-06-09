#include "hades/transaction.hpp"

namespace hades
{
	bool transaction::commit()
	{
		const auto begin = std::begin(_data);
		const auto end = std::end(_data);
		auto iter = begin;
		for (iter; iter != end; ++iter)
		{
			const auto r = std::invoke(iter->exchange_lock, iter->data);
			if (!r)
				break;
		}

		if (iter == end)
		{
			for (auto iter2 = begin; iter2 != end; ++iter2)
				std::invoke(iter->commit, iter->data);

			return true;
		}
		else
		{
			for (auto iter2 = begin; iter2 != iter; ++iter2)
				std::invoke(iter2->release, iter->data);
		}

		return false;
	}

	void transaction::abort() noexcept
	{
		_data.clear();
	}
}
