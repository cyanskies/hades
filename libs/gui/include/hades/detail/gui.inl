#include "hades/gui.hpp"

#include <cstring>
#include <functional>
#include <type_traits>

#include "misc/cpp/imgui_stdlib.h"

namespace hades
{
	template<size_t Count>
	inline void gui::indent()
	{
		std::size_t i = 0u;
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

	template<typename Container>
		requires string_type<typename Container::value_type>
	bool gui::listbox(std::string_view label,
		std::size_t& current_item, const Container& container, const vector2& size)
	{
		const auto to_string_view = [](auto&& s) noexcept -> std::string_view {
			return s;
		};

		return listbox(label, current_item, container, to_string_view, size);
	}

	template<typename Container, typename ToString>
		requires string_type<std::invoke_result_t<ToString, typename Container::value_type>>
	bool gui::listbox(std::string_view label, std::size_t& current_item,
		const Container& container, ToString to_string_func, const vector2& size)
	{
		auto changed = false;
		if (listbox_begin(label, size))
		{
			const auto s = std::size(container);
			for (auto i = std::size_t{}; i < s; ++i)
			{
				if (selectable(std::invoke(to_string_func, container[i]), i == current_item))
				{
					current_item = i;
					changed = true;
				}
			}

			listbox_end();
		}

		return changed;
	}

	namespace detail
	{
		template<typename T, std::enable_if_t<gui_supported_types<T>, int> = 0>
		constexpr ImGuiDataType get_imgui_data_type() noexcept
		{
			if constexpr (std::is_same_v<T, int8>)
				return ImGuiDataType_S8;
			else if constexpr (std::is_same_v<T, uint8>)
				return ImGuiDataType_U8;
			else if constexpr (std::is_same_v<T, int16>)
				return ImGuiDataType_S16;
			else if constexpr (std::is_same_v<T, uint16>)
				return ImGuiDataType_U16;
			else if constexpr (std::is_same_v<T, int32>)
				return ImGuiDataType_S32;
			else if constexpr (std::is_same_v<T, uint32>)
				return ImGuiDataType_U32;
			else if constexpr (std::is_same_v<T, int64>)
				return ImGuiDataType_S64;
			else if constexpr (std::is_same_v<T, uint64>)
				return ImGuiDataType_U64;
			else if constexpr (std::is_same_v<T, float>)
				return ImGuiDataType_Float;
			else if constexpr (std::is_same_v<T, double>)
				return ImGuiDataType_Double;
			else
				return ImGuiDataType_COUNT;
		}
	}

	template<std::size_t N, std::enable_if_t<N < 5, int>>
	bool gui::slider_float(std::string_view label, std::array<float, N>& v, float min, float max, slider_flags flags, std::string_view format)
	{
		if constexpr (N == 0) return;
		if constexpr (N == 1)
			return slider_float(label, v[0], min, max, flags, format);

		_active_assert();
		constexpr auto slider_float_n = [] {
			if constexpr (N == 2)
				return ImGui::SliderFloat2;
			else if constexpr (N == 3)
				return ImGui::SliderFloat3;
			else if constexpr (N == 4)
				return ImGui::SliderFloat4;
		}();
		return std::invoke(slider_float_n, to_string(label).c_str(), v.data(), min, max, to_string(format).c_str(), enum_type(flags));
	}

	template<std::size_t N, std::enable_if_t< N < 5, int>>
	bool gui::slider_int(std::string_view label, std::array<int, N>& v, int min, int max, slider_flags flags, std::string_view format)
	{
		if constexpr (N == 0) return;
		if constexpr (N == 1)
			return slider_int(label, v[0], min, max, flags, format);

		_active_assert();
		constexpr auto slider_int_n = [] {
			if constexpr (N == 2)
				return ImGui::SliderInt2;
			else if constexpr (N == 3)
				return ImGui::SliderInt3;
			else if constexpr (N == 4)
				return ImGui::SliderInt4;
		}();
		return std::invoke(slider_int_n, to_string(label).c_str(), v.data(), min, max, to_string(format).c_str(), enum_type(flags));
	}

	template<typename T, typename U, std::enable_if_t<detail::gui_supported_types<T>, int>>
	bool gui::slider_scalar(std::string_view label, T &v, U min, U max, std::string_view fmt, slider_flags f)
	{
		static_assert(std::is_convertible_v<U, T>, "The types of min and max must be convertible to T");
		_active_assert();

		auto min_t = integer_cast<T>(min);
		auto max_t = integer_cast<T>(max);

		return ImGui::SliderScalar(to_string(label).c_str(), detail::get_imgui_data_type<T>(),
			&v, &min_t, &max_t, !empty(fmt) ? to_string(fmt).c_str() : nullptr, enum_type(f));
	}

	namespace detail
	{
		template<typename T>
		using input_imp_return = std::enable_if_t<!is_string_v<T>, bool>;

		template<typename T>
		using input_imp_string_return = std::enable_if_t<is_string_v<T>, bool>;

		//basic imp, calls input scalar
		template<typename T>
		inline input_imp_return<T> input_imp(gui &g, std::string_view label, T &v, gui::input_text_flags f)
		{
			static_assert(detail::valid_input_scalar_v<T>, "gui::input_scalar only accepts int, float and double");
			constexpr auto step = static_cast<T>(1);
			return g.input_scalar<T>(label, v, step, step, f);
		}
	
		//string imp
		template<typename T>
		inline input_imp_string_return<T> input_imp(gui &g, std::string_view label, T &v, gui::input_text_flags f)
		{
			using Type = std::decay_t<T>;
			if constexpr (std::is_same_v<Type, std::string_view>)
				static_assert(always_false_v<Type>, "string_view is not mutable");
			else if constexpr (std::is_same_v<Type, char*>)
			{
				return ImGui::InputText(to_string(label).data(), v, std::strlen(v), static_cast<ImGuiInputTextFlags>(f));
			}
			else
			{
				return g.input_text(label, v, f);
			}
		}

		//array imp, calls input scalar array
		template<typename T, std::size_t Size>
		inline bool input_imp(gui &g, std::string_view l, std::array<T, Size> &v, gui::input_text_flags f)
		{
			static_assert(detail::gui_supported_types<T>, "Detected gui::input() as an array input, but the array type isn't supported");
			constexpr auto step = static_cast<T>(1);
			return g.input_scalar<T>(l, v, step, step, f);
		}

		//specialisation for static sized char array buffers
		template<std::size_t Size>
		inline bool input_imp(gui &g, std::string_view l, std::array<char, Size> &v, gui::input_text_flags f)
		{
			return g.input_text(l, v, f);
		}
	}

	template<typename T>
	inline bool gui::input(std::string_view label, T &v, input_text_flags f)
	{
		_active_assert();
		return detail::input_imp(*this, label, v, f);
	}

	template<std::size_t Size, typename Callback>
	inline bool gui::input_text(std::string_view label, std::array<char, Size>& buffer, input_text_flags f, Callback cb)
	{
		_active_assert();
		if constexpr (std::is_null_pointer_v<Callback>)
			return ImGui::InputText(to_string(label).data(), buffer.data(), buffer.size(), static_cast<ImGuiInputTextFlags>(f));
		else
		{
			static_assert(std::is_invocable_v<Callback, gui_input_text_callback&>);
			auto invoke_callback = [](ImGuiInputTextCallbackData* data) {
				auto callback = static_cast<Callback*>(data->UserData);
				auto input_data = gui_input_text_callback{ data };
				std::invoke(*callback, input_data);
				return 0;
			};

			return ImGui::InputText(to_string(label).data(), buffer.data(),
				buffer.size(), static_cast<ImGuiInputTextFlags>(f), invoke_callback, &cb);
		}
	}

	template<typename Callback>
	bool gui::input_text(std::string_view label, std::string& buffer, input_text_flags f, Callback cb)
	{
		_active_assert();
		if constexpr(std::is_null_pointer_v<Callback>)
			return ImGui::InputText(to_string(label).data(), &buffer, static_cast<ImGuiInputTextFlags>(f));
		else
		{
			static_assert(std::is_invocable_v<Callback, gui_input_text_callback&>);
			auto invoke_callback = [](ImGuiInputTextCallbackData* data) {
				auto callback = static_cast<Callback*>(data->UserData);
				auto input_data = gui_input_text_callback{ data };
				std::invoke(*callback, input_data);
				return 0;
			};
			
			return ImGui::InputText(to_string(label).data(), &buffer,
				static_cast<ImGuiInputTextFlags>(f), invoke_callback, &cb);
		}
	}

	template<std::size_t Size>
	inline bool gui::input_text_multiline(std::string_view label, std::array<char, Size>& buffer, const vector2 &size, input_text_flags f)
	{
		_active_assert();
		return ImGui::InputTextMultiline(to_string(label).data(), buffer.data(), buffer.size(), {size.x, size.y}, static_cast<ImGuiInputTextFlags>(f));
	}

	template<typename T, std::enable_if_t<detail::gui_supported_types<T>, int>>
	inline bool gui::input_scalar(std::string_view label, T &v, std::optional<T> step, std::optional<T> step_fast, input_text_flags f)
	{
		_active_assert();

		constexpr auto type = detail::get_imgui_data_type<T>();
		const char* format = NULL;
		if constexpr (type == ImGuiDataType_Float)
			format = "%.3f";
		else if constexpr (type == ImGuiDataType_Double)
			format = "&.6f";

		const T* const step_ptr = step ? &*step : nullptr;
		const T* const step_fast_ptr = step_fast ? &*step_fast : nullptr;

		return ImGui::InputScalar(to_string(label).data(), type, &v, step_ptr, step_fast_ptr, format, static_cast<ImGuiInputTextFlags>(f));
	}

	template<typename T, std::size_t N, std::enable_if_t<detail::gui_supported_types<T>, int>>
	bool gui::input_scalar(std::string_view label, std::array<T, N>& v, std::optional<T> step, std::optional<T> step_fast, input_text_flags f)
	{
		_active_assert();

		constexpr auto type = detail::get_imgui_data_type<T>();
		const char* format = NULL;
		if constexpr (type == ImGuiDataType_Float)
			format = "%.3f";
		else if constexpr (type == ImGuiDataType_Double)
			format = "%.6f";

		const T* const step_ptr = step ? &*step : nullptr;
		const T* const step_fast_ptr = step_fast ? &*step_fast : nullptr;

		return ImGui::InputScalarN(to_string(label).data(), type, v.data(), integer_cast<int>(size(v)), step_ptr, step_fast_ptr, format, static_cast<ImGuiInputTextFlags>(f));
	}

	template<typename InputIt, typename MakeButton>
	void gui_make_horizontal_wrap_buttons(gui &g, float right_max, InputIt first, InputIt last, MakeButton&& make_button)
	{
		using T = typename std::iterator_traits<InputIt>::value_type;
		using TRef = typename std::iterator_traits<InputIt>::reference;
		using TPtr = typename std::iterator_traits<InputIt>::pointer;

		constexpr auto button_size = vector_float{ 25.f, 25.f };

		g.indent();

		const auto indent_amount = g.get_item_rect_max().x;

		auto x2 = 0.f;
		while (first != last)
		{
			const auto new_x2 = x2 + button_size.x;
			if (indent_amount + new_x2 < right_max)
				g.layout_horizontal();
			else
				g.indent();
			
			if constexpr (std::is_invocable_v<MakeButton, gui&, T> ||
				std::is_invocable_v<MakeButton, gui&, TRef> ||
				std::is_invocable_v<MakeButton, gui&, const TRef>)
				std::invoke(make_button, g, *first++);
			else if constexpr (std::is_invocable_v<MakeButton, gui&, TPtr> ||
				std::is_invocable_v<MakeButton, gui&, const TPtr>)
				std::invoke(make_button, g, &*first++);
			else
				static_assert(always_false_v<MakeButton>, "MakeButton must have the following definition: void(gui&, T); where T can be any of(T, T&, const T&, T*, const T*");

			x2 = g.get_item_rect_max().x;
		}
	}

	bool gui::set_dragdrop_payload(detail::DragDropPayload auto payload, std::string_view type_name, set_condition_enum cond)
	{
		_active_assert();
		assert("This type requires a manually provided type name" && !empty(type_name));
		assert("Type name must be shorter than 32 chars" && size(type_name) < 32);
		return ImGui::SetDragDropPayload(to_string(type_name).c_str(),
			&payload, sizeof(payload), enum_type(cond));
	}

	template<detail::DragDropPayload T>
	gui_dragdrop_payload<T> gui::accept_dragdrop_payload(std::string_view type_name, dragdrop_flags flags)
	{
		_active_assert();
		return { ImGui::AcceptDragDropPayload(to_string(type_name).c_str(), enum_type(flags)) };
	}

}

