#ifndef HADES_LEVEL_EDITOR_COMPONENT_HPP
#define HADES_LEVEL_EDITOR_COMPONENT_HPP

#include <functional>
#include <tuple>

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/level.hpp"
#include "hades/timers.hpp"
#include "hades/types.hpp"
#include "hades/vector_math.hpp"

namespace hades::resources
{
	struct animation;
}

namespace hades::editor::gui_names
{
	using namespace std::string_view_literals;
	//Window name for left toolbox window
	constexpr auto toolbox = "##toolbox"sv;
	constexpr auto new_level = "New Level"sv;
}

namespace hades::editor
{
	using namespace std::string_view_literals;
	constexpr auto new_level_name = "A brand new level"sv;
	constexpr auto new_level_description = "An empty level, ready to be filled with content"sv;
}

namespace hades
{
	class gui;
	struct mission;
	
	class level_editor_component
	{
	public:
		struct editor_windows
		{
			bool new_level = false;
			bool load_level = false;
			bool save_level = false;
		};

		virtual ~level_editor_component() noexcept = default;

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

		virtual level level_new(level l) const { return l; };
		virtual void level_load(const level&) {};
		virtual level level_save(level l) const { return l; };
		//TODO: level_resize

		virtual void gui_update(gui&, editor_windows&) {};

		//mouse position, in world coords
		using mouse_pos = vector_float;

		//used to generate info for draw_brush_preview
		virtual void make_brush_preview(time_duration, mouse_pos) {};

		//return true if their is a valid target for dragging under the mouse
		virtual bool valid_drag_target(mouse_pos) const { return false; };

		virtual void on_click(mouse_pos) {};
		
		virtual void on_drag_start(mouse_pos) {};
		virtual void on_drag(mouse_pos) {};
		virtual void on_drag_end(mouse_pos) {};

		virtual void draw(sf::RenderTarget&, time_duration, sf::RenderStates) const {};

		virtual void draw_brush_preview(sf::RenderTarget&, time_duration, sf::RenderStates) const {};

	private:
		activate_brush_f _activate_brush;
	};
}

#endif //!HADES_LEVEL_EDITOR_COMPONENT_HPP
