#ifndef HADES_LEVEL_EDITOR_OBJECTS_HPP
#define HADES_LEVEL_EDITOR_OBJECTS_HPP

#include <variant>

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/level_editor_component.hpp"
#include "hades/objects.hpp"
#include "hades/quad_map.hpp"
#include "hades/resource_base.hpp"
#include "hades/sprite_batch.hpp"

namespace hades::resources
{
	struct level_editor_object_settings_t {};

	struct level_editor_object_settings : resource_type<level_editor_object_settings_t>
	{
		using object_group = std::tuple<string, std::vector<const object*>>;
		std::vector<object_group> groups;
		sf::Color object_colour = sf::Color::Cyan;
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
		void make_brush_preview(time_duration, mouse_pos) override;
		void draw_brush_preview(sf::RenderTarget&, time_duration, sf::RenderStates) const override;

	private:
		enum class brush_type {
			object_place,
			object_selector,
			object_drag,
			region_selector,
			region_place
		};

		struct editor_object_instance final : object_instance
		{
			editor_object_instance() noexcept = default;
			editor_object_instance(const object_instance&);

			sprite_batch::sprite_id _id = sprite_utility::sprite::bad_sprite_id;
		};

		brush_type _brush_type{ brush_type::object_selector };
		const resources::level_editor_object_settings *_settings = nullptr;
		editor_object_instance _held_object;
		//_held_animation
		std::variant<sf::Sprite, sf::RectangleShape> _held_preview;

		//objects for drawing
		sprite_batch _sprites;
		//object instances
		std::vector<editor_object_instance> _objects;
		quad_tree<entity_id> _quad;
		//TODO: name list
	};
}

#endif //!HADES_LEVEL_EDITOR_OBJECTS_HPP
