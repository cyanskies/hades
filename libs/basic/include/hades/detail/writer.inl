#include "hades/writer.hpp"

#include <ranges>

namespace hades::data
{
	template<typename T, string_transform<T> ToString>
	void writer::mergable_sequence(std::string_view key, const std::vector<T>& current, const std::vector<T>& prev, ToString&& func)
	{
		using namespace std::string_view_literals;

		assert(std::ranges::is_sorted(current));
		assert(std::ranges::is_sorted(prev));
		// list containing the elements in prev, that are absent from current;
		auto removed = std::vector<T>{ size(prev), T{} };
		// list containing the elements in current, that weren't present in prev
		auto added = std::vector<T>{ size(current), T{} };
		const auto [prev_end, removed_end] = std::ranges::set_difference(prev, current, begin(removed));
		const auto [current_end, added_end] = std::ranges::set_difference(current, prev, begin(added));

		// trim any unused space
		removed.erase(removed_end, end(removed));
		added.erase(added_end, end(added));

		// nothing was changed
		if (empty(removed) && empty(added))
			return;

		if (empty(key))
			start_sequence();
		else
			start_sequence(key);

		// if removed = prev, then everything was replaced
		// use [=]
		if (removed == prev && !empty(prev))
			write("="sv);
		else if (!empty(removed)) // list removals
			write("-"sv);

		auto to_str = make_to_string_functor<T>(std::forward<ToString>(func));

		for (const auto& elm : removed)
			write(std::invoke(to_str, elm));

		if (!empty(removed) && !empty(added)) // restore addition mode, if it was disabled
			write("+"sv);

		for (const auto& elm : added)
			write(std::invoke(to_str, elm));
		
		end_sequence();
		return;
	}

	template<typename T, string_transform<T> ToString>
	void writer::sequence(std::string_view key, const std::vector<T>& collection, ToString&& func)
	{
		if (empty(key))
			start_sequence();
		else
			start_sequence(key);

		auto to_str = make_to_string_functor<T>(std::forward<ToString>(func));

		for (const auto& elm : collection)
			write(std::invoke(to_str, elm));
		end_sequence();
		return;
	}
}