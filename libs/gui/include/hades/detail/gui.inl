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
	inline detail::listbox_with_string<Container> gui::listbox(std::string_view label,
		std::size_t &current_item, const Container& container, int height_in_items)
	{
		using T = typename Container::value_type;
		static_assert(is_string_v<T>);
		static_assert(!std::is_same_v<T, std::string_view>);

		_active_assert();

		//convert the current item into an int to pass into IMGUI
		auto current_as_int = integer_cast<int>(current_item);

		//NOTE: container is not modified, but ListBox() only accepts non-const
		auto &cont = const_cast<Container&>(container);

		const auto result = ImGui::ListBox(to_string(label).data(), &current_as_int,
			[](void *data, int index, const char **out_text)->bool {
			assert(index >= 0);
			const auto i = integer_cast<std::size_t>(index);
			const auto &container = *static_cast<Container*>(data);
			if (i > std::size(container))
				return false;

			auto at = std::begin(container);
			std::advance(at, i);

			*out_text = [](auto &&val)->const char* {
				if constexpr (std::is_same_v<string, std::decay_t<decltype(val)>>)
					return val.c_str();
				else
					return val; // must be a char*
			}(*at);
			return true;
		}, &cont, integer_cast<int32>(std::size(container)), height_in_items);

		//update the current_item ref
		assert(current_as_int >= 0);
		current_item = integer_cast<std::size_t>(current_as_int);

		return result;
	}

	template<typename Container>
	inline detail::listbox_no_string<Container> gui::listbox(std::string_view label,
		std::size_t &current_item, const Container &container, int height_in_items)
	{
		using T = typename Container::value_type;
		using hades::to_string;
		return listbox(label, current_item, container,
			[](T value) { return to_string(value); },
			height_in_items);
	}
	
	namespace detail
	{
		static thread_local void* to_string_stash = {};
		static thread_local string listbox_string_buffer;
		template<typename Container, typename ToString>
		bool listbox_helper_func(void* data, int idx, const char** out_text)
		{
			const auto& func = *static_cast<ToString*>(to_string_stash);
			const auto& container = *static_cast<const Container*>(data);
			if (idx >= size(container))
				return false;

			auto iter = begin(container);
			iter = std::next(iter, idx);
			listbox_string_buffer = std::invoke(func, *iter);
			*out_text = listbox_string_buffer.c_str();
			return empty(listbox_string_buffer);
		}
	}

	template<typename Container, typename ToString>
		requires std::regular_invocable<ToString, typename Container::value_type>
	bool gui::listbox(std::string_view lable, std::size_t& current_item,
		const Container& data, ToString to_string_func, int height_in_items)
	{
		using T = typename Container::value_type;
		static_assert(std::is_invocable_r_v<string, ToString, const T&>,
			"ToString must accept the list entry type and return a string");

		auto int_item = integer_cast<int>(current_item);
		const auto item_count = integer_cast<int>(size(data));
		detail::to_string_stash = &to_string_func;
		//NOTE: container is not modified, but ImGui::ListBox() only accepts non-const
		auto& cont = const_cast<Container&>(data);
		const auto ret = ImGui::ListBox(to_string(lable).c_str(), &int_item,
			detail::listbox_helper_func<Container, ToString>, &cont, item_count, height_in_items);
		current_item = integer_cast<std::size_t>(int_item);

		#ifndef NDEBUG
		// clear out the thread globals
		detail::to_string_stash = {};
		detail::listbox_string_buffer = {};
		#endif // !NDEBUG

		return ret;
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
}

