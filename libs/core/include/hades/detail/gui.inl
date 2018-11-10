#include "..\gui.hpp"

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

		constexpr auto func = [] {
			if constexpr (std::is_same_v<T, int>)
				return &ImGui::InputInt;
			else if constexpr (std::is_same_v<T, float>)
				return &ImGui::InputFloat;
			else if constexpr (std::is_same_v<T, double>)
				return &ImGui::InputDouble;
		}();

		return std::invoke(func, to_string(label).data(), &v, step, step_fast, static_cast<ImGuiInputTextFlags>(f));
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

