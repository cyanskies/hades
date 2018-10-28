#ifndef HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP
#define HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP

#include "hades/level_editor_component.hpp"
#include "hades/types.hpp"

namespace hades
{
	class level_editor_level_props final : public level_editor_component
	{
	public:
		void gui_update(gui&) override;
		void gui_menubar(menu_functions);
		void gui_window();
	private:
		string _level_name;
		string _level_desc;
	};
}

#endif //!HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP
