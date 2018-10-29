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
		const auto main_menu_created = _gui.main_menubar_begin();
		assert(main_menu_created);

		if (_gui.menu_begin("file"sv))
		{
			_gui.menu_item("new..."sv);
			_gui.menu_item("load..."sv);
			_gui.menu_item("save"sv);
			_gui.menu_item("save as..."sv);
			_gui.menu_end();
		}

		_gui.main_menubar_end();

		//make toolbar
		const auto toolbar_created = _gui.main_toolbar_begin();
		assert(toolbar_created);
		const auto toolbar_y2 = _gui.window_position().y + _gui.window_size().y;
		_gui.main_toolbar_end();

		//make toolbox window
		assert(_toolbox_width);
		_gui.next_window_position({ 0.f, toolbar_y2 });
		const auto toolbox_size = [](int32 width, float window_width)->float {
			constexpr auto auto_width = -1;
			if (width == auto_width)
				return window_width / 3;
			else
				return static_cast<float>(width);
		}(*_toolbox_width, _window_width);

		_gui.next_window_size({ toolbox_size, _window_height - toolbar_y2 });
		const auto toolbox_created = _gui.window_begin("##toolbox", gui::window_flags::panel);
		assert(toolbox_created);
		_gui.window_end();

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

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_hand_component_setup()
	{
		for_each_tuple(_editor_components, [this](auto &&c, std::size_t index) {
			auto activate_brush = [this, index] {
				_active_brush = index;
			};

			c.install_callbacks(
				activate_brush
			);
		});
	}
}