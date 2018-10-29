#ifndef HADES_LEVEL_EDITOR_OBJECTS_HPP
#define HADES_LEVEL_EDITOR_OBJECTS_HPP

#include "hades/level_editor_component.hpp"

namespace hades
{
	class level_editor_objects final : public level_editor_component
	{
	public:
		void gui_update(gui&) override;

	private:
		enum class brush_type {
			object_place,
			object_selector,
			region_selector,
			region_place
		};

		brush_type _brush_type{ brush_type::object_selector };

		//objects for drawing
		//object instances
	};
}

#endif //!HADES_LEVEL_EDITOR_OBJECTS_HPP
