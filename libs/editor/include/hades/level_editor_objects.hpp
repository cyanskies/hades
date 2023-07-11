#ifndef HADES_LEVEL_EDITOR_OBJECTS_HPP
#define HADES_LEVEL_EDITOR_OBJECTS_HPP

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/collision_grid.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/level_editor_grid.hpp"
#include "hades/objects.hpp"
#include "hades/object_editor.hpp"
#include "hades/resource_base.hpp"
#include "hades/sprite_batch.hpp"

namespace hades::resources
{
	struct level_editor_object_settings_t {};

	struct level_editor_object_settings : resource_type<level_editor_object_settings_t>
	{
		void serialise(const data::data_manager&, data::writer&) const override;

		using object_group = std::tuple<string, std::vector<resource_link<object>>>;
		std::vector<object_group> groups;
		static constexpr auto default_colour = sf::Color::Cyan;
		sf::Color object_colour = default_colour;
	};
}

namespace hades
{
	void register_level_editor_object_resources(data::data_manager&);

	class level_editor_objects_impl : public level_editor_component
	{
	public:
		struct editor_object_instance final : object_instance
		{
			editor_object_instance() noexcept = default;
			editor_object_instance(const object_instance&);

			sprite_batch::sprite_id sprite_id = sprite_utility::bad_sprite_id;
		};

		using object_collision_grid = uniform_collision_grid<entity_id, rect_float>;
		using collision_layer_map = std::unordered_map<unique_id, object_collision_grid>;

		level_editor_objects_impl();

		void level_load(const level&) override final;
		level level_save(level l) const override final;
		void level_resize(vector_int, vector_int) override final;

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
		const object_collision_grid& get_quadmap() const noexcept;
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
		void _remove_object_callback(entity_id);
		void _remove_object(entity_id);
		bool _try_place_object(vector_float pos, editor_object_instance);
		void _update_changed_obj(editor_object_instance& o);
		void _update_quad_data(const object_instance& o);

		std::vector<const resources::object*> _object_types;
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
		vector_float _level_limit = {};
		float _collision_grid_cell_size;
		std::optional<object_collision_grid> _quad_selection; // quadtree used for selecting objects
		//quadtree used for object collision
		collision_layer_map _collision_quads;

		//object instances
		using obj_data_type = obj_ui::object_data<editor_object_instance>;
		using obj_ui_type = object_editor<obj_data_type,
			std::function<void(editor_object_instance*)>,
			std::function<void(entity_id)>>;

		obj_data_type _objects;
		obj_ui_type _obj_ui;

		//grid info for snapping
		grid_vars _grid = get_console_grid_vars();
	};

	class level_editor_objects final : public level_editor_objects_impl
	{
	public:
		using level_editor_objects_impl::level_editor_objects_impl;
	};
}

#endif //!HADES_LEVEL_EDITOR_OBJECTS_HPP
