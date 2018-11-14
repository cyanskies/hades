#ifndef HADES_LEVEL_EDITOR_OBJECTS_HPP
#define HADES_LEVEL_EDITOR_OBJECTS_HPP

#include <optional>
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

		void level_load(const level&) override;

		void gui_update(gui&, editor_windows&) override;
		void make_brush_preview(time_duration, mouse_pos) override;
		void draw_brush_preview(sf::RenderTarget&, time_duration, sf::RenderStates) const override;

		void on_click(mouse_pos) override;

		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) const override;

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

			sprite_batch::sprite_id sprite_id = sprite_utility::sprite::bad_sprite_id;
		};

		void _update_position(const object_instance&, vector_float);
		void _update_size(const object_instance&, vector_float);
		void _update_pos_size(entity_id, vector_float, vector_float);

		bool _show_objects = true;
		bool _allow_intersect = false;
		bool _show_regions = true;
		brush_type _brush_type{ brush_type::object_selector };
		const resources::level_editor_object_settings *_settings = nullptr;
		std::optional<editor_object_instance> _held_object;
		//_held_animation
		std::variant<sf::Sprite, sf::RectangleShape> _held_preview;

		//objects for drawing
		sprite_batch _sprites;
		//object instances
		entity_id::value_type _next_id = static_cast<entity_id::value_type>(bad_entity) + 1;
		std::vector<editor_object_instance> _objects;
		quad_tree<entity_id, rect_float> _quad;
		vector_float _level_limit;
		//TODO: name list
	};
}

#endif //!HADES_LEVEL_EDITOR_OBJECTS_HPP
