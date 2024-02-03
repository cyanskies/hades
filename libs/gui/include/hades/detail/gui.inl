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
        {
			active_selection = this_button;
            return true;
        }
        return false;
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

	template<typename Container, string_transform<typename Container::value_type> ToString>
	bool gui::listbox(std::string_view label, std::size_t& current_item, const Container& container,
		ToString&& to_string_func, const vector2& size)
	{
		auto changed = false;
		if (listbox_begin(label, size))
		{
			const auto s = std::size(container);
			const auto to_str = make_to_string_functor<typename Container::value_type>(std::forward<ToString>(to_string_func));
			for (auto i = std::size_t{}; i < s; ++i)
			{
				if (selectable(to_str(container[i]), i == current_item))
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
		template<gui_supported_type T>
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

		template<detail::gui_supported_type T>
		constexpr const char* imgui_scalar_fmt() noexcept
		{
			if constexpr (std::floating_point<T>)
				return "%.3f";
			else if constexpr (std::integral<T>)
				return "%d";
		}
	}

	template<std::size_t N, std::enable_if_t<N < 5, int>>
	bool gui::slider_float(std::string_view label, std::array<float, N>& v, float min, float max, slider_flags flags, std::string_view format)
	{
        if constexpr (N == 0) return false;
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
		return std::invoke(slider_float_n, label, v.data(), min, max, to_string(format).c_str(), enum_type(flags));
	}

	template<std::size_t N, std::enable_if_t< N < 5, int>>
	bool gui::slider_int(std::string_view label, std::array<int, N>& v, int min, int max, slider_flags flags, std::string_view format)
	{
        if constexpr (N == 0) return false;
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
		return std::invoke(slider_int_n, label, v.data(), min, max, to_string(format).c_str(), enum_type(flags));
	}

	template<detail::gui_supported_type T>
	bool gui::slider_scalar(std::string_view label, T &v, T min, T max, const std::string& fmt, slider_flags f)
	{
		_active_assert();
		return ImGui::SliderScalar(label, detail::get_imgui_data_type<T>(),
			&v, &min, &max,
			!empty(fmt) ? to_string(fmt).c_str() : detail::imgui_scalar_fmt<T>(),
			enum_type(f));
	}

	template<detail::gui_supported_type T, std::size_t N>
	bool gui::slider_scalar(std::string_view label, std::array<T, N>& v, T min, T max, const std::string& fmt, slider_flags f)
	{
		_active_assert();
		return ImGui::SliderScalarN(label, detail::get_imgui_data_type<T>(),
			v.data, integer_cast<int>(N), &min, &max,
			!empty(fmt) ? to_string(fmt).c_str() : detail::imgui_scalar_fmt<T>(),
			enum_type(f));
	}

	template<detail::gui_supported_type T>
	bool gui::vertical_slider_scalar(std::string_view label, const vector2& size, T& v, T min, T max, const std::string& fmt, slider_flags f)
	{
		_active_assert();
		return ImGui::VSliderScalar(label, { size.x, size.y }, detail::get_imgui_data_type<T>(),
			&v, &min, &max,
			!empty(fmt) ? to_string(fmt).c_str() : detail::imgui_scalar_fmt<T>(),
			enum_type(f));
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
			static_assert(detail::gui_supported_type<T>, "Detected gui::input() as an array input, but the array type isn't supported");
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

	template<detail::gui_supported_type T>
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

	template<detail::gui_supported_type T, std::size_t N>
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

