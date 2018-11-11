#ifndef HADES_LEVEL_EDITOR_GRID_HPP
#define HADES_LEVEL_EDITOR_GRID_HPP

#include "hades/level_editor_component.hpp"

namespace hades::data
{
	class data_manager;
}

namespace hades
{
	void register_level_editor_grid_variables();
	void register_level_editor_grid_resource(data::data_manager&);

	//provides an interface for editing level name, properties, background colour
	//TODO: background image(stretch, repeat, paralax)
	class level_editor_grid final : public level_editor_component
	{
	};
}

namespace hades::cvars
{
	using namespace std::string_view_literals;
}

#endif //!HADES_LEVEL_EDITOR_GRID_HPP
