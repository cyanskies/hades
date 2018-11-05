#ifndef HADES_LEVEL_EDITOR_OBJECTS_HPP
#define HADES_LEVEL_EDITOR_OBJECTS_HPP

#include "hades/level_editor_component.hpp"
#include "hades/objects.hpp"
#include "hades/resource_base.hpp"

namespace hades::resources
{
	struct level_editor_object_settings_t {};

	struct level_editor_object_settings : resource_type<level_editor_object_settings_t>
	{
		using object_group = std::tuple<string, std::vector<const object*>>;
		std::vector<object_group> groups;
	};
}

namespace hades
{
	void register_level_editor_object_resources(data::data_manager&);

	class level_editor_objects final : public level_editor_component
	{
	public:
		level_editor_objects();

		void gui_update(gui&) override;

	private:
		enum class brush_type {
			object_place,
			object_selector,
			region_selector,
			region_place
		};

		brush_type _brush_type{ brush_type::object_selector };
		const resources::level_editor_object_settings *_settings = nullptr;
		std::vector<bool> _object_group_selection;
		//objects for drawing
		//object instances
	};
}

#endif //!HADES_LEVEL_EDITOR_OBJECTS_HPP
