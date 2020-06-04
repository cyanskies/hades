#include "hades/gui.hpp"

#include <cstring>
#include <type_traits>

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
		return listbox(label, current_item, container, to_string<T>,
			height_in_items);
	}

	template<typename Container, typename ToString>
	inline bool gui::listbox(std::string_view label, std::size_t &current_item,
		const Container &container, ToString to_string_func,
		int height_in_items)
	{
		using T = typename Container::value_type;
		static_assert(std::is_invocable_r_v<string, ToString, const T&>,
			"ToString must accept the list entry type and return a string");

		auto strings = std::vector<string>{};
		strings.reserve(std::size(container));

		std::transform(std::begin(container), std::end(container),
			std::back_inserter(strings), to_string_func);

		return listbox(label, current_item, strings, height_in_items);
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
			return g.input_scalar(label, v, step, step, f);
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
				return g.input_text(to_string(label).data(), v, f);
			}
		}

		//array imp, calls input scalar array
		template<typename T, std::size_t Size>
		inline bool input_imp(gui &g, std::string_view l, std::array<T, Size> &v, gui::input_text_flags f)
		{
			return g.input_scalar_array(l, v, f);
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
	inline bool gui::input_scalar(std::string_view label, T &v, T step, T step_fast, input_text_flags f)
	{
		_active_assert();

		static_assert(detail::valid_input_scalar_v<T>, "gui::input_scalar only accepts int, float and double");

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
	inline bool gui::input_scalar_array(std::string_view label, std::array<T, Size> &v, input_text_flags f)
	{
		_active_assert();

		static_assert(detail::valid_input_scalar_v<T>,
			"gui::input_scalar_array only accepts std::array<int>, std::array<float> and std::array<double>");

		constexpr auto data_type = [] {
			if constexpr (std::is_same_v<T, int>)
				return ImGuiDataType_::ImGuiDataType_S32;
			else if constexpr (std::is_same_v<T, float>)
				return ImGuiDataType_::ImGuiDataType_Float;
			else if constexpr (std::is_same_v<T, double>)
				return ImGuiDataType_::ImGuiDataType_Double;
		}();

		constexpr auto fmt_str = [] {
			if constexpr (std::is_same_v<T, int>)
				return "%d";
			else if constexpr (std::is_same_v<T, float>)
				return "%.3f";
			else if constexpr (std::is_same_v<T, double>)
				return "%.6f";
		}();

		const auto components = v.size();
		const auto int_components = integer_cast<int>(components);
		assert(signed_cast(components) == int_components);

		return ImGui::InputScalarN(to_string(label).data(), data_type, v.data(), int_components, NULL, NULL, fmt_str, static_cast<ImGuiInputTextFlags>(f));
	}

	template<typename InputIt, typename MakeButton>
	void gui_make_horizontal_wrap_buttons(gui &g, float right_max, InputIt first, InputIt last, MakeButton make_button)
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

