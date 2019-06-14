#include "hades/transaction.hpp"

#include "hades/utility.hpp"

namespace hades
{
	bool transaction::commit()
	{
		const auto begin = std::begin(_data);
		const auto end = std::end(_data);
		auto iter = begin;
		for (iter; iter != end; ++iter)
		{
			//TODO: check that function ptrs were set
			const auto r = std::invoke(iter->exchange_lock, iter->data);
			if (!r)
				break;
		}

		const auto finally = make_finally([this]() noexcept {
			_data.clear();
		});

		if (iter == end)
		{
			for (iter = begin; iter != end; ++iter)
				std::invoke(iter->commit, iter->data);

			return true;
		}

		return false;
	}

	void transaction::abort() noexcept
	{
		_data.clear();
	}
}
