#ifndef HADES_LEVEL_EDITOR_OBJECTS_HPP
#define HADES_LEVEL_EDITOR_OBJECTS_HPP

#include <any>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/level_editor_component.hpp"
#include "hades/level_editor_grid.hpp"
#include "hades/objects.hpp"
#include "hades/properties.hpp"
#include "hades/quad_map.hpp" // TODO: switch to collision grid
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

		void set_selected(entity_id) noexcept;
		ObjectType* get_obj(entity_id) noexcept;
		const ObjectType* get_obj(entity_id) const noexcept;
		entity_id add();
		entity_id add(ObjectType);
		void erase(entity_id);

		struct curve_edit_cache
		{
			string edit_buffer;
			int32 edit_generation = 0;
			std::any extra_data;
		};

		using cache_map = std::unordered_map<string, curve_edit_cache>;

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

		struct add_remove_curve_window
		{
			enum class window_state {
				closed,
				add,
				remove
			};

			window_state state = window_state::closed;
			std::size_t list_index = std::size_t{};
		};

		void _add_remove_curve_window(gui&);
		void _erase(std::size_t);
		std::size_t _get_obj(entity_id) const noexcept;
		ObjectType* _get_obj(std::size_t) noexcept;
		void _reset_add_remove_curve_window() noexcept;
		template<typename MakeRect>
		void _positional_property_field(gui&, std::string_view, ObjectType&, 
			curve_info&, MakeRect&&);
		void _property_editor(gui&);
		void _set_selected(std::size_t);

		//callbacks
		OnChange _on_change{};
		IsValidPos _is_valid_pos{};

		//shared state
		object_data *_data = nullptr;

		//editing and ui data
		std::size_t _obj_list_selected = std::size_t{};
		std::size_t _next_added_object_base = std::size_t{};
		entity_id _selected = bad_entity;
		add_remove_curve_window _add_remove_window_state;
		std::string _entity_name_id_uncommited;
		vector_curve_edit _vector_curve_edit;
		cache_map _edit_cache;
		std::array<curve_info, 2> _curve_properties;
	};

	class level_editor_objects_impl : public level_editor_component
	{
	public:
		struct editor_object_instance final : object_instance
		{
			editor_object_instance() noexcept = default;
			editor_object_instance(const object_instance&);

			sprite_batch::sprite_id sprite_id = sprite_utility::bad_sprite_id;
		};

		using object_collision_tree = quad_tree<entity_id, rect_float>;
		using collision_layer_map = std::unordered_map<unique_id, object_collision_tree>;

		level_editor_objects_impl();

		void level_load(const level&) override final;
		level level_save(level l) const override final;
		//void level_resize(vector_int, vector_int) override; //TODO:

		void gui_update(gui&, editor_windows&) override final;
		void make_brush_preview(time_duration, mouse_pos) override final;
		void draw_brush_preview(sf::RenderTarget&, time_duration, sf::RenderStates) override final;

		tag_list get_object_tags_at_location(rect_float) const override final;

		void on_click(mouse_pos) override final;
		void on_drag_start(mouse_pos) override final;
		void on_drag(mouse_pos) override final;
		void on_drag_end(mouse_pos) override final;

		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override final;

	protected:
		const std::vector<editor_object_instance>& get_objects() const noexcept;
		const object_collision_tree& get_quadmap() const noexcept;
		const collision_layer_map& get_collision_layers() const noexcept;
		world_vector_t get_level_size() const noexcept;

		// behaviour override functions
		// inherit from this class and override these to change editor behaviour

		//NOTE: for custom world snapping(eg. snap to terrain grid),
		//		default does collision layer avoidance
		//		it returns pos if it is clear of other objects
		//		and a default constructed std::optional otherwise
		virtual std::optional<world_vector_t> closest_valid_position(world_rect_t pos, const object_instance&) const;
		virtual void on_object_place(object_instance&) {};
		virtual void set_grid_settings_for_object_type(grid_vars&, object_instance&);
		//virtual create_update_object_sprite(editor_object_instance&, sprite_batch&);

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
		unique_id _object_owner = unique_zero;

		//objects for drawing
		std::variant<sf::Sprite, sf::RectangleShape> _held_preview;
		sprite_batch _sprites;
		//level info
		vector_float _level_limit;
		object_collision_tree _quad_selection; // quadtree used for selecting objects
		//quadtree used for object collision
		collision_layer_map _collision_quads;

		//object instances
		using obj_ui = object_editor_ui<editor_object_instance,
			std::function<void(editor_object_instance&)>,
			std::function<bool(const rect_float & r, const object_instance & o)>>;
		obj_ui::object_data _objects;
		obj_ui _obj_ui;

		//grid info for snapping
		grid_vars _grid = get_console_grid_vars();
	};

	class level_editor_objects final : public level_editor_objects_impl
	{
	public:
		using level_editor_objects_impl::level_editor_objects_impl;
	};
}

#include "hades/detail/level_editor_objects.inl"

#endif //!HADES_LEVEL_EDITOR_OBJECTS_HPP
