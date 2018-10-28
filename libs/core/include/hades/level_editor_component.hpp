#ifndef HADES_LEVEL_EDITOR_COMPONENT_HPP
#define HADES_LEVEL_EDITOR_COMPONENT_HPP

#include <functional>
#include <tuple>

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/timers.hpp"
#include "hades/types.hpp"

namespace hades::resources
{
	struct animation;
}

namespace hades
{
	class gui;
	struct level;

	class level_editor_component
	{
	public:
		virtual ~level_editor_component() noexcept = default;

		struct menu_functions
		{
			//bool specifies whether the item or menu is enabled
			//bool& holds the toggle value
			using begin_f = std::function<bool(std::string_view, bool)>;
			using end_f = std::function<void(void)>;
			using item_f = std::function<bool(std::string_view, bool)>;
			using toggle_item_f = std::function<bool(std::string_view, bool&, bool)>;

			begin_f begin;
			end_f end;
			item_f item;
			toggle_item_f toggle_item;
		};

		//callbacks for toolbar buttons
		struct toolbar_functions
		{
			using toolbar_button_f = std::function<bool(std::string_view)>;
			using toolbar_icon_button_f = std::function<bool(const resources::animation*)>;
			using toolbar_seperator_f = std::function<void(void)>;

			toolbar_button_f button;
			toolbar_icon_button_f icon_button;
			toolbar_seperator_f seperator;
		};

		//generic callbacks, these are always available
		using activate_brush_f = std::function<void(void)>;

		template<typename ActivateBrush>
		void install_callbacks(ActivateBrush ab)
		{
			_activate_brush = ab;
		}

		void activate_brush()
		{
			_activate_brush();
		}

		virtual void gui_update(gui&) {};
		virtual void gui_menubar(menu_functions) {};
		virtual void gui_toolbar(toolbar_functions) {};
		virtual void gui_toolbox() {};
		virtual void gui_info_window(level&) {};

		//mouse position, in world coords
		using mouse_pos = std::tuple<int32, int32>;

		//used to generate info for draw_brush_preview
		virtual void make_brush_preview(mouse_pos) {};

		//return true if their is a valid target for dragging under the mouse
		virtual bool valid_drag_target(mouse_pos) const { return false; };

		virtual void on_click(mouse_pos) {};
		
		virtual void on_drag_start(mouse_pos) {};
		virtual void on_drag(mouse_pos) {};
		virtual void on_drag_end(mouse_pos) {};

		virtual void draw(sf::RenderTarget&, time_duration, sf::RenderStates) const {};

		virtual void draw_brush_preview() const {};

		//TODO: onsave, onload

	private:
		activate_brush_f _activate_brush;
	};
}

#endif //!HADES_LEVEL_EDITOR_COMPONENT_HPP
