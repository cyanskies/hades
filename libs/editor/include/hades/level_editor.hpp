#ifndef HADES_LEVEL_EDITOR_HPP
#define HADES_LEVEL_EDITOR_HPP

#include <string_view>
#include <tuple>

#include "SFML/Graphics/RenderTexture.hpp"

#include "hades/gui.hpp"
#include "hades/level.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/level_editor_grid.hpp"
#include "hades/level_editor_objects.hpp"
#include "hades/level_editor_level_properties.hpp"
#include "hades/level_editor_regions.hpp"
#include "hades/level_editor_terrain.hpp"
#include "hades/properties.hpp"
#include "hades/mouse_input.hpp"
#include "hades/state.hpp"
#include "hades/tiles.hpp"
#include "hades/types.hpp"
#include "hades/vector_math.hpp"

//TODO: add noexcept wherever possible

namespace hades
{
	class mission_editor_t;
}

namespace hades::detail
{
	constexpr auto mouse_drag_enabled = true;
	constexpr auto mouse_double_click_enabled = false;

	// TODO: templatize level on the map type and then templatize this on level
	//			this will let us support classical layered tilemaps in a more natural way
	class level_editor_impl : public state
	{
	public:
		using brush_index_t = std::size_t;
		constexpr static auto invalid_brush = std::numeric_limits<brush_index_t>::max();

		using level_type = level;
		// conditional based on level::map_type
		using map_type = mutable_terrain_map;

		explicit level_editor_impl(const std::filesystem::path& = {});
		level_editor_impl(const mission_editor_t*, level*);

		~level_editor_impl() noexcept override = default;

		void init() override;
		bool handle_event(const event&) override;
		void reinit() override;

		level get_level() const;

		void update(time_duration, const sf::RenderTarget&, const input_system::action_set&) override;
		void draw(sf::RenderTarget&, time_duration) override;

		using get_players_return_type = level_editor_component::get_players_return_type;
		get_players_return_type get_players() const;

	protected:
		virtual level _component_on_new(level) const = 0;
		virtual void _component_on_load(const level&) = 0;
		virtual level _component_on_save(level) const = 0;
		virtual void _component_on_resize(vector2_int, vector2_int) = 0;
		virtual void _component_on_reinit(vector2_float, vector2_float) = 0;
		virtual void _component_on_click(brush_index_t, std::optional<terrain_target>) = 0;
		virtual void _component_on_drag_start(brush_index_t, std::optional<terrain_target>) = 0;
		virtual void _component_on_drag(brush_index_t, std::optional<terrain_target>) = 0;
		virtual void _component_on_drag_end(brush_index_t, std::optional<terrain_target>) = 0;
		virtual void _component_on_rotate(float) = 0;
		virtual void _component_on_screen_move(rect_float) = 0;
		virtual void _component_height_toggle(bool) = 0;
		virtual void _draw_components(sf::RenderTarget&, time_duration, brush_index_t, sf::RenderStates = {}) = 0;
		virtual void _generate_brush_preview(brush_index_t brush_index, time_duration, std::optional<terrain_target> world_position) = 0;
		virtual void _handle_component_setup() = 0;
		void _set_active_brush(std::size_t index) noexcept;
		bool _is_active_brush(std::size_t index) const noexcept;
		virtual void _update_component_gui(gui&, level_editor_component::editor_windows&) = 0;

		float _get_world_rot() const noexcept
		{
			return _accumulated_rotation;
		}

		map_type& _get_map() noexcept
		{
			assert(_map);
			return *_map;
		}

		tag_list _get_terrain_tags_at_location(rect_float) const;

	private:
		sf::View _gui_view;
		sf::View _world_view;

		sf::Transform _world_transform;

		console::property_int _camera_height;
		console::property_bool _rotate_enabled;
		console::property_int _toolbox_width;
		console::property_int _toolbox_auto_width;

		console::property_int _view_height;

		console::property_int _scroll_margin;
		console::property_float _scroll_rate;
		
		//current window size
		float _rotate_widget = {};
		float _accumulated_rotation = {};
		float _zoom = 1.f;
		float _window_width = 0.f, _window_height = 0.f;
		// TODO: remove this min values and associated logic 
		int32 _left_min = 0, _top_min = 0; // minimum values for world interaction(represents the edge of the UI)

		//level width, height
		level_size_t _level_x = 0, _level_y = 0;

	private:
		struct new_level_opt
		{
			int32 width, height;
		};

		struct resize_opt
		{
			vector2_int size;
			vector2_int offset;
			vector2_int top_left;
			vector2_int bottom_right;
		};

		bool _mission_mode() const;
		void _load(level*);
		void _recalculate_camera();
		void _save();
		void _update_gui(time_duration);

		std::string _mouse_pos_world;
		std::string _mouse_pos_world_f;
		// states the cliff layer under the mouse
		// TODO: add a cliff layer overlay to terrain_map
		//		need to add a region overlay to it anyway
		std::string _cliff_layer; 
		bool _error_modal = false;
		string _error_msg = "no error";

		level_editor_component::editor_windows _window_flags;
		gui _gui = { "level_editor.ini" };
		std::unique_ptr<level> _level_ptr; //used in level mode
		// store map in a ptr so it has a stable address for the terrain component to keep
		std::unique_ptr<map_type> _map = std::make_unique<map_type>();
		const mission_editor_t* _mission_editor = nullptr;
		level *_level = nullptr;
		const resources::terrain_settings* _terrain_settings = resources::get_terrain_settings();
		mouse::mouse_button_state<mouse_drag_enabled, mouse_double_click_enabled> _mouse_left;
		std::optional<terrain_target> _last_drag;

		//new level options
		new_level_opt _new_level_options = {};
		console::property_int _force_whole_tiles;

		resize_opt _resize_options = {};
		string _load_level_mod;
		string _load_level_path;
		string _next_save_path = "new_level.lvl";
		string _save_path;

		sf::RectangleShape _background;

		time_point _total_run_time;

		//currently active brush
		brush_index_t _active_brush = invalid_brush;
		terrain_mini_map _minimap; 
		sf::RenderTexture _minimap_render_texture;

		bool _height_enabled = true;
	};
}

namespace hades
{
	void create_editor_console_variables();

	template<typename ...Components>
	class basic_level_editor final : public detail::level_editor_impl
	{
	public:
		using detail::level_editor_impl::level_editor_impl;

	private:
		level _component_on_new(level) const override;
		void _component_on_load(const level&) override;
		level _component_on_save(level) const override;
		void _component_on_resize(vector2_int, vector2_int) override;
		void _component_on_reinit(vector2_float, vector2_float) override;
		//returns the sum of tags reported by components for that location
		//may contain duplicates
		tag_list _component_get_object_tags_at_location(rect_float) const;
		void _component_on_click(brush_index_t, std::optional<terrain_target>) override;
		void _component_on_drag_start(brush_index_t, std::optional<terrain_target>) override;
		void _component_on_drag(brush_index_t, std::optional<terrain_target>) override;
		void _component_on_drag_end(brush_index_t, std::optional<terrain_target>) override;
		void _component_on_rotate(float) override;
		void _component_on_screen_move(rect_float) override;
		void _component_height_toggle(bool) override;

		void _draw_components(sf::RenderTarget&, time_duration, brush_index_t, sf::RenderStates = {}) override;
		void _generate_brush_preview(brush_index_t, time_duration, std::optional<terrain_target>) override;
		void _handle_component_setup() override;
		void _update_component_gui(gui&, level_editor_component::editor_windows&) override;

		using component_tuple = std::tuple<Components...>;
		component_tuple _editor_components;
	};

	//standard level editor
	using level_editor = basic_level_editor<
		level_editor_level_props,
		level_editor_terrain,
		level_editor_objects,
		level_editor_regions
	>;

	// these register all the needed resources and console vars to use the 
	// level_editor defined above
	// NOTE: it is always safe to register or create the same resources, or console var twice
	void register_level_editor_resources(data::data_manager&);
	void create_level_editor_console_vars();
}

namespace hades::cvars
{
	constexpr auto editor_toolbox_width = "editor_toolbox_width"; // set to -1 for auto screen width
	constexpr auto editor_toolbox_auto_width = "editor_toolbox_auto_width"; // used to calculate the toolbox width in auto mode
																			// window width / auto_width; lower value makes a larger toolbox
	constexpr auto editor_scroll_margin_size = "editor_scroll_margin_size";
	constexpr auto editor_scroll_rate = "editor_scroll_rate";

	constexpr auto editor_camera_height_px = "editor_camera_height";

	constexpr auto editor_zoom_max = "editor_zoom_max"; // zoom min, is 0
	constexpr auto editor_zoom_default = "editor_zoom_default";

	//force_whole_tiles
	// controls the limits for map creation and resize
	// forces the new size in tiles rather than game units
	// 0 off
	// 1 on
	constexpr auto editor_level_force_whole_tiles = "editor_level_force_whole_tiles";
	constexpr auto editor_level_default_size = "editor_level_default_size";

	constexpr auto editor_rotate_world = "editor_rotate_world";
}

namespace hades::cvars::default_value
{
	constexpr auto editor_toolbox_width = -1;
	constexpr auto editor_toolbox_auto_width = 4;

	constexpr auto editor_scroll_margin_size = 20;
	constexpr auto editor_scroll_rate = 4.f;

	constexpr auto editor_camera_height_px = 320;

	constexpr auto editor_zoom_max = 4;
	constexpr auto editor_zoom_default = 2;

	constexpr auto editor_level_force_whole_tiles = 0;
	constexpr auto editor_level_default_size = 512;

	constexpr auto editor_rotate_world = true;
}

#include "hades/detail/level_editor.inl"

#endif //!HADES_LEVEL_EDITOR_HPP
