#include "hades/gui.hpp"

#include <type_traits>

namespace hades
{
	template<size_t Count>
	inline void gui::indent()
	{
		size_t i = 0u;
		while (i < Count)
		{
			dummy();
			layout_horizontal();
			++i;
		}
	}

	template<typename T>
	bool gui::radio_button(std::string_view label, T &active_selection, T this_button)
	{
		_active_assert();
		if (radio_button(label, active_selection == this_button))
			active_selection = this_button;
	}

	template<template<typename> typename Container, typename T, typename>
	inline bool gui::listbox(std::string_view label, int32 &current_item, Container<T> container, int32 height_in_items)
	{
		static_assert(is_string_v<T>);
		static_assert(!std::is_same_v<T, std::string_view>);

		_active_assert();

		return ImGui::ListBox(to_string(label).data(), &current_item,
			[](void *data, int index, const char **out_text)->bool {
			assert(dynamic_cast<Container<T>*>(data));

			auto &container = *static_cast<Container<T>*>(data);
			if (index < 0 || index > std::size(container))
				return false;

			auto at = std::begin(container);
			std::advance(at, index);

			*out_text = [](auto &&val)->const char* {
				if constexpr (std::is_same_v < string, std::decay_t<decltype(value)>)
					return val.c_str();
				else
					return val; // must be a char*
			}(*at);
			return true;
		}, &container, std::size(container), height_in_items);
	}

	template<template<typename> typename Container, typename T>
	inline bool gui::listbox(std::string_view label, int32 &current_item, Container<T> container, int32 height_in_items)
	{
		return listbox(label, current_item, to_string<T>, container,
			std::size(container), height_in_items);
	}

	template<template<typename> typename Container, typename T, typename ToString>
	inline bool gui::listbox(std::string_view label, int32 &current_item, ToString to_string_func, Container<T> container, int32 height_in_items)
	{
		static_assert(std::is_invocable_r_v<string, ToString, const T&>,
			"ToString must accept the list entry type and return a string");

		auto strings = std::vector<string>{};
		const auto size = std::size(container)
		strings.reserve(size);

		std::transform(std::begin(container), std::end(container),
			std::back_inserter(strings), to_string_func);

		return listbox(label, current_item, strings, size,
			height_in_items);
	}

	template<std::size_t Size>
	inline bool gui::input_text(std::string_view label, std::array<char, Size>& buffer, input_text_flags f)
	{
		_active_assert();
		return ImGui::InputText(to_string(label).data(), buffer.data(), buffer.size(), static_cast<ImGuiInputTextFlags>(f));
	}

	template<std::size_t Size>
	inline bool gui::input_text_multiline(std::string_view label, std::array<char, Size>& buffer, const vector &size, input_text_flags)
	{
		_active_assert();
		return ImGui::InputTextMultiline(to_string(label).data(), buffer.data(), buffer.size(), {size.x, size.y}, static_cast<ImGuiInputTextFlags>(f));
	}

	template<typename T>
	inline bool gui::input(std::string_view label, T &v, T step, T step_fast, input_text_flags f)
	{
		_active_assert();

		constexpr auto func = [&] {
			if constexpr (std::is_same_v<T, int>)
				return [&](auto &v, auto &step, auto &step_fast) {
				return ImGui::InputInt(to_string(label).data(), &v, step, step_fast, static_cast<ImGuiInputTextFlags>(f));
			};
			else if constexpr (std::is_same_v<T, float>)
				return [&](auto &v, auto &step, auto &step_fast) {
				return ImGui::InputFloat(to_string(label).data(), &v, step, step_fast, "%.3f", static_cast<ImGuiInputTextFlags>(f));
			};
			else if constexpr (std::is_same_v<T, double>)
				return [&](auto &v, auto &step, auto &step_fast) {
				return ImGui::InputDouble(to_string(label).data(), &v, step, step_fast, "%.6f", static_cast<ImGuiInputTextFlags>(f));
			};
		}();

		return std::invoke(func, v, step, step_fast);
	}

	template<typename T, std::size_t Size>
	inline bool gui::input(std::string_view label, std::array<T, Size> &v, input_text_flags f)
	{
		_active_assert();

		constexpr auto data_type = [] {
			if constexpr (std::is_same_v<T, int>)
				return ImGuiDataType_S32;
			else if constexpr (std::is_same_v<T, float>)
				return ImGuiDataType_Float;
			else if constexpr (std::is_same_v<T, double>)
				return ImGuiDataType_Double;
		}();

		constexpr auto fmt_str = [] {
			if constexpr (std::is_same_v<T, int>)
				return "%d";
			else if constexpr (std::is_same_v<T, float>)
				return "%%.%df";
			else if constexpr (std::is_same_v<T, double>)
				return "%.6f";
		}();

		return InputScalarN(to_string(label).data(), data_type, v.data(), v.size(), NULL, NULL, fmt_str, static_cast<ImGuiInputTextFlags>(f));
	}
}

