#ifndef HADES_LEVEL_EDITOR_COMPONENT_HPP
#define HADES_LEVEL_EDITOR_COMPONENT_HPP

#include <functional>

namespace hades::resources
{
	struct animation;
}

namespace hades
{
	class level_editor_component
	{
	public:
		virtual ~level_editor_component() noexcept = default;

		using toolbar_button_f = std::function<bool(std::string_view)>;
		using toolbar_icon_button_f = std::function<bool(const resources::animation*)>;
		using toolbar_seperator_f = std::function<void(void)>;

		virtual void gui_toolbar(toolbar_button_f, toolbar_icon_button_f, toolbar_seperator_f) {};
		virtual void gui_toolbox_tab();
		virtual void gui_info_window();
	};
}

#endif //!HADES_LEVEL_EDITOR_COMPONENT_HPP
