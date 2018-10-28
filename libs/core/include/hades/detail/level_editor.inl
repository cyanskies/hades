#include "hades/level_editor.hpp"

#include "hades/animation.hpp"
#include "hades/properties.hpp"
#include "hades/console_variables.hpp"
#include "hades/for_each_tuple.hpp"

namespace hades
{
	template<typename ...Components>
	inline void basic_level_editor<Components...>::init()
	{
		for_each_tuple(_editor_components, [this](auto &&c, std::size_t index) {
			auto activate_brush = [this, index] {
				_active_brush = index;
			};

			c.install_callbacks(
				activate_brush
			);
		});

		reinit();
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::update(time_duration delta_time, const sf::RenderTarget &, input_system::action_set)
	{
		_gui.update(delta_time);
		_gui.frame_begin();

		using namespace std::string_view_literals;

		//make main menu
		_gui.main_menubar_begin();

		if (_gui.menu_begin("file"sv))
		{
			_gui.menu_item("test"sv);
			_gui.menu_end();
		}

		for_each_tuple(_editor_components, [this](auto &&c) {
			auto begin_menu = [this](std::string_view s, bool e) {
				_gui.menu_begin(s, e);
			};

			auto end_menu = [this]{
				_gui.menu_end();
			};

			auto menu_item = [this](std::string_view s, bool e) {
				_gui.menu_item(s, e);
			};

			auto menu_item_toggle = [this](std::string_view s, bool &a, bool e) {
				_gui.menu_toggle_item(s, a, e);
			};

			//c.gui_menubar({ begin_menu, end_menu, menu_item, menu_item_toggle });
		});

		//store menubar size for later
		const auto &main_menubar_size = _gui.get_item_rect_max();
		_gui.main_menubar_end();

		//make toolbar
		_gui.next_window_position({ 0.f, main_menubar_size.y }); //TODO: account for main_menu size
		_gui.next_window_size({ _window_width, 0.f });

		using flags = gui::window_flags;
		constexpr auto toolbar_flags =
			flags::no_collapse |
			flags::no_move |
			flags::no_titlebar |
			flags::no_saved_settings;
		_gui.window_begin("toolbar"sv, toolbar_flags);

		auto last_button_x2 = 0.f;
		for_each_tuple(_editor_components, [&, this](auto &&comp) {
			const auto &style = ImGui::GetStyle();
			const auto frame_pad = style.FramePadding.x * 2;
			const auto item_spacing = style.ItemSpacing.x;
			constexpr auto button_size = gui::vector{ 25.f, 25.f };
			constexpr auto layout_size = gui::vector{ 0.f, -1.f };
			auto button = [&, this](std::string_view str)
			{
				//const auto size = _gui.calculate_text_size(str).x + frame_pad;
				const auto size = button_size.x;
				const auto next_button_x2 = size + item_spacing + last_button_x2;
				if (next_button_x2 < _window_width) _gui.layout_horizontal(layout_size.x, layout_size.y);
				const auto result = _gui.button(str, button_size);
				last_button_x2 = _gui.get_item_rect_max().x;

				return result;
			};

			auto icon_button = [&, this](const resources::animation* a)
			{
				const auto size = button_size.x; 
				const auto next_button_x2 = size + item_spacing + last_button_x2;
				if (next_button_x2 < _window_width) _gui.layout_horizontal(layout_size.x, layout_size.y);
				const auto result = _gui.image_button(*a, button_size);
				last_button_x2 = _gui.get_item_rect_max().x;

				return result;
			};

			auto seperator = [&, this]()
			{
				const auto size = 1.f;
				const auto next_button_x2 = size + item_spacing + last_button_x2;
				if (next_button_x2 < _window_width) _gui.layout_horizontal(layout_size.x, layout_size.y);
				_gui.separator_horizontal();
				last_button_x2 = _gui.get_item_rect_max().x;
			};

			comp.gui_toolbar({ button, icon_button, seperator });

			seperator();
		});

		_gui.window_end(); //toolbar;

		//make toolbox
		//make infobox
		//make minimap

		for_each_tuple(_editor_components, [this](auto &&c) {
			c.gui_update(_gui);
		});

		_gui.frame_end();
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::draw(sf::RenderTarget &target, time_duration delta_time)
	{
		auto states = sf::RenderStates{};
		for_each_tuple(_editor_components, [&target, &delta_time, states](auto &&v) {
			v.draw(target, delta_time, states);
		});

		target.draw(_gui);
	}
}