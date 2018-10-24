#ifndef HADES_LEVEL_EDITOR_COMPONENT_HPP
#define HADES_LEVEL_EDITOR_COMPONENT_HPP

#include <functional>

namespace hades::resources
{
	struct animation;
}

namespace hades
{
	struct level;

	class level_editor_component
	{
	public:
		virtual ~level_editor_component() noexcept = default;

		//callbacks for toolbar buttons
		using toolbar_button_f = std::function<bool(std::string_view)>;
		using toolbar_icon_button_f = std::function<bool(const resources::animation*)>;
		using toolbar_seperator_f = std::function<void(void)>;

		//generic callbacks, these are always available
		using activate_brush_f = std::function<void(void)>;

		template<typename ActivateBrush>
		void install_callbacks(ActivateBrush ab)
		{
			activate_brush_f = ab;
		}

		virtual void gui_toolbar(toolbar_button_f, toolbar_icon_button_f, toolbar_seperator_f) {};
		virtual void gui_toolbox();
		virtual void gui_info_window(level&);

	protected:
		activate_brush_f activate_brush;
	};
}

#endif //!HADES_LEVEL_EDITOR_COMPONENT_HPP
