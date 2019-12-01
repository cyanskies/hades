#ifndef HADES_LEVEL_EDITOR_OBJECTS_HPP
#define HADES_LEVEL_EDITOR_OBJECTS_HPP

#include <optional>
#include <unordered_set>
#include <variant>

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/level_editor_component.hpp"
#include "hades/level_editor_grid.hpp"
#include "hades/objects.hpp"
#include "hades/properties.hpp"
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

	template<typename ObjectType = object_instance, typename OnChange = nullptr_t, typename IsValidPos = nullptr_t>
	class object_editor_ui
	{
	public:
		static_assert(std::is_base_of_v<object_instance, ObjectType> ||
			std::is_same_v<object_instance, ObjectType>);

		static_assert(std::is_same_v<OnChange, nullptr_t> ||
			std::is_invocable_v<OnChange, ObjectType&>,
			"If the OnChange callback is provided it must have the following signiture: auto OnChange(ObjectType&)");

		static_assert(std::is_same_v<IsValidPos, nullptr_t> ||
			std::is_invocable_r_v<bool, IsValidPos, const rect_float&, const object_instance&>,
			"If the IsValidPos callback is provided it must have the following signiture: bool IsValidPos(const rect_float&, const object_instance&)");

		static_assert(!std::is_same_v<OnChange, IsValidPos> || 
			std::is_same_v<IsValidPos, nullptr_t> && std::is_same_v<OnChange, nullptr_t>,
			"If one of the editor callbacks are provided, then both must be.");

		static constexpr bool visual_editor = !std::is_same_v<OnChange, nullptr_t>;

		struct object_data
		{
			std::vector<ObjectType> objects;
			std::unordered_map<string, entity_id> entity_names;
			entity_id next_id = next(bad_entity);
		};

		struct vector_curve_edit
		{
			std::size_t selected = 0;
			const resources::curve* target = nullptr;
		};

		explicit object_editor_ui(object_data* d) noexcept
			: _data{d}
		{
			assert(_data);
		}

		object_editor_ui(object_data* d, OnChange change_func, IsValidPos isvalid_func)
		: _data{ d }, _on_change{ change_func }, _is_valid_pos{ isvalid_func }
		{
			assert(_data);
		}

		void show_object_list_buttons(gui&);
		void object_list_gui(gui&);
		void object_properties(gui&);

		void set_selected(entity_id id) noexcept;
		ObjectType* get_obj(entity_id) noexcept;
		void erase(entity_id);

	private:
		struct curve_info
		{
			const resources::curve* curve = nullptr;
			resources::curve_default_value value;
		};

		enum class curve_index : std::size_t {
			pos,
			size_index
		};

		std::size_t _get_obj(entity_id) noexcept;
		template<typename MakeRect>
		void _positional_property_field(gui&, std::string_view, ObjectType&, 
			curve_info&, MakeRect);
		void _property_editor(gui&);
		void _set_selected(std::size_t);

		//callbacks
		OnChange _on_change;
		IsValidPos _is_valid_pos;

		//shared state
		object_data *_data = nullptr;

		//editing and ui data
		std::size_t _obj_list_selected;
		entity_id _selected = bad_entity;
		std::string _entity_name_id_uncommited;
		vector_curve_edit _vector_curve_edit;
		std::array<curve_info, 2> _curve_properties;

	};

	class level_editor_objects final : public level_editor_component
	{
	public:
		struct editor_object_instance final : object_instance
		{
			editor_object_instance() noexcept = default;
			editor_object_instance(const object_instance&);

			sprite_batch::sprite_id sprite_id = sprite_utility::bad_sprite_id;
		};

		using object_collision_tree = quad_tree<entity_id, rect_float>;

		level_editor_objects();

		void level_load(const level&) override;
		level level_save(level l) const override;
		//void level_resize(vector_int, vector_int) override; //TODO:

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

		bool _object_valid_location(const rect_float&, const object_instance&) const;
		//removes object
		void _remove_object(entity_id);
		bool _try_place_object(vector_float pos, editor_object_instance);
		void _update_changed_obj(editor_object_instance& o);
		void _update_quad_data(const object_instance& o);

		//editing settings and state
		bool _show_objects = true;
		bool _allow_intersect = false;
		bool _show_regions = true;
		brush_type _brush_type{ brush_type::object_selector };
		const resources::level_editor_object_settings* _settings = nullptr;
		std::optional<editor_object_instance> _held_object;
		//objects for drawing
		std::variant<sf::Sprite, sf::RectangleShape> _held_preview;
		sprite_batch _sprites;
		//level info
		vector_float _level_limit;
		object_collision_tree _quad_selection; // quadtree used for selecting objects
		//quadtree used for object collision
		std::unordered_map<unique_id, object_collision_tree> _collision_quads;
		//object instances
		using obj_ui = object_editor_ui<editor_object_instance, 
			std::function<void(editor_object_instance&)>,
			std::function<bool(const rect_float & r, const object_instance & o)>>;

		obj_ui::object_data _objects;
		obj_ui _obj_ui;

		//grid info for snapping
		grid_vars _grid = get_console_grid_vars();
	};
}

#include "hades/detail/level_editor_objects.inl"

#endif //!HADES_LEVEL_EDITOR_OBJECTS_HPP
