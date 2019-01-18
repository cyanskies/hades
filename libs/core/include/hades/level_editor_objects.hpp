#ifndef HADES_LEVEL_EDITOR_OBJECTS_HPP
#define HADES_LEVEL_EDITOR_OBJECTS_HPP

#include <optional>
#include <unordered_set>
#include <variant>

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/level_editor_component.hpp"
#include "hades/objects.hpp"
#include "hades/properties.hpp"
#include "hades/quad_map.hpp"
#include "hades/resource_base.hpp"
#include "hades/sprite_batch.hpp"

// TODO: 
// Add support for regions

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
		struct editor_object_instance final : object_instance
		{
			editor_object_instance() noexcept = default;
			editor_object_instance(const object_instance&);

			sprite_batch::sprite_id sprite_id = sprite_utility::sprite::bad_sprite_id;
		};

		struct curve_info
		{
			const resources::curve* curve = nullptr;
			resources::curve_default_value value;
		};

		struct vector_curve_edit
		{
			std::size_t selected = 0;
			const resources::curve *target = nullptr;
		};

		using object_collision_tree = quad_tree<entity_id, rect_float>;

		level_editor_objects();

		void level_load(const level&) override;
		level level_save(level l) const override;

		void gui_update(gui&, editor_windows&) override;
		void make_brush_preview(time_duration, mouse_pos) override;
		void draw_brush_preview(sf::RenderTarget&, time_duration, sf::RenderStates) override;

		tag_list get_tags_at_location(rect_float) const override;

		void on_click(mouse_pos) override;
		void on_drag_start(mouse_pos) override;
		void on_drag(mouse_pos) override;
		void on_drag_end(mouse_pos) override;

		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override;

	private:
		enum class brush_type {
			object_place,
			object_selector,
			object_multiselect,
			object_drag,
		};

		enum curve_index : std::size_t {
			pos_x,
			pos_y,
			size_x,
			size_y
		};

		vector_float _calculate_position(vector_float pos) const;
		void _make_property_editor(gui&);
		template<typename MakeBoundRect, typename SetChangedProperty>
		void _make_positional_property_edit_field(gui&, std::string_view,
			editor_object_instance&, curve_info&, MakeBoundRect, SetChangedProperty);

		bool _object_valid_location(const rect_float&, const object_instance&) const;
		bool _object_valid_location(vector_float pos, vector_float size, const object_instance&) const;
		//removes object
		void _remove_object(entity_id);
		bool _try_place_object(vector_float pos, editor_object_instance);
		void _update_quad_data(const object_instance &o);

		//editing settings and state
		bool _show_objects = true;
		bool _allow_intersect = false;
		bool _show_regions = true;
		brush_type _brush_type{ brush_type::object_selector };
		const resources::level_editor_object_settings *_settings = nullptr;
		std::optional<editor_object_instance> _held_object;
		//objects for drawing
		std::variant<sf::Sprite, sf::RectangleShape> _held_preview;
		sprite_batch _sprites;
		//level info
		entity_id::value_type _next_id = static_cast<entity_id::value_type>(bad_entity) + 1;
		vector_float _level_limit;
		//object instances
		std::vector<editor_object_instance> _objects;
		object_collision_tree _quad_selection; // quadtree used for selecting objects
		//quadtree used for object collision
		std::unordered_map<unique_id, object_collision_tree> _collision_quads;
		//object data for editing
		std::unordered_map<string, entity_id> _entity_names; 
		std::string _entity_name_id_uncommited;
		vector_curve_edit _vector_curve_edit;
		std::array<curve_info, 4> _curve_properties;

		//grid info for snapping
		console::property_bool _grid;
		console::property_bool _grid_snap;
		console::property_bool _grid_auto;
		console::property_float _grid_size;
		console::property_int _grid_step;
		console::property_int _grid_step_max;
	};
}

#include "hades/detail/level_editor_objects.inl"

#endif //!HADES_LEVEL_EDITOR_OBJECTS_HPP
