#ifndef HADES_LEVEL_EDITOR_HPP
#define HADES_LEVEL_EDITOR_HPP

#include <string_view>
#include <tuple>

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

	class level_editor_impl : public state
	{
	public:
		using brush_index_t = std::size_t;
		constexpr static auto invalid_brush = std::numeric_limits<brush_index_t>::max();

		level_editor_impl();
		level_editor_impl(const mission_editor_t*, level*);

		~level_editor_impl() noexcept override = default;

		void init() override;
		bool handle_event(const event&) override;
		void reinit() override;

		level get_level() const;

		void update(time_duration, const sf::RenderTarget&, input_system::action_set) override;
		void draw(sf::RenderTarget&, time_duration) override;

		using get_players_return_type = level_editor_component::get_players_return_type;
		get_players_return_type get_players() const;

	protected:
		virtual level _component_on_new(level) const = 0;
		virtual void _component_on_load(const level&) = 0;
		virtual level _component_on_save(level) const = 0;
		virtual void _component_on_resize(vector2_int, vector2_int) = 0;
		virtual void _component_on_click(brush_index_t, vector2_float) = 0;
		virtual void _component_on_drag_start(brush_index_t, vector2_float) = 0;
		virtual void _component_on_drag(brush_index_t, vector2_float) = 0;
		virtual void _component_on_drag_end(brush_index_t, vector2_float) = 0;
		virtual void _draw_components(sf::RenderTarget&, time_duration, brush_index_t) = 0;
		virtual void _generate_brush_preview(brush_index_t brush_index, time_duration, vector2_float world_position) = 0;
		virtual void _handle_component_setup() = 0;
		void _set_active_brush(std::size_t index) noexcept;
		virtual void _update_component_gui(gui&, level_editor_component::editor_windows&) = 0;

	private:
		//current window size
		float _window_width = 0.f, _window_height = 0.f;
		int32 _left_min = 0, _top_min = 0; // minimum values for world interaction(represents the edge of the UI)

		console::property_int _camera_height;
		console::property_int _toolbox_width;
		console::property_int _toolbox_auto_width;

		console::property_int _scroll_margin;
		console::property_float _scroll_rate;

		//level width, height
		level_size_t _level_x = 0, _level_y = 0;

		sf::View _gui_view;
		sf::View _world_view;

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
		void _save();
		void _update_gui(time_duration);

		bool _error_modal = false;
		string _error_msg = "no error";

		level_editor_component::editor_windows _window_flags;
		gui _gui = { "level_editor.ini" };
		std::unique_ptr<level> _level_ptr; //used in level mode
		const mission_editor_t* _mission_editor = nullptr;
		level *_level = nullptr;
		mouse::mouse_button_state<mouse_drag_enabled, mouse_double_click_enabled> _mouse_left;

		//new level options
		new_level_opt _new_level_options = {};
		console::property_int _force_whole_tiles;
		const resources::tile_settings* _tile_settings =
			data::try_get<resources::tile_settings>(resources::get_tile_settings_id()).result;

		resize_opt _resize_options = {};
		string _load_level_mod;
		string _load_level_path;
		string _next_save_path = "new_level.lvl";
		string _save_path;

		sf::RectangleShape _background;

		time_point _total_run_time;

		//currently active brush
		brush_index_t _active_brush = invalid_brush;
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
		//returns the sum of tags reported by components for that location
		//may contain duplicates
		tag_list _component_get_object_tags_at_location(rect_float) const;
		tag_list _component_get_terrain_tags_at_location(rect_float) const;
		void _component_on_click(brush_index_t, vector2_float) override;
		void _component_on_drag_start(brush_index_t, vector2_float) override;
		void _component_on_drag(brush_index_t, vector2_float) override;
		void _component_on_drag_end(brush_index_t, vector2_float) override;
		void _draw_components(sf::RenderTarget&, time_duration, brush_index_t) override;
		void _generate_brush_preview(brush_index_t, time_duration, vector2_float) override;
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
		level_editor_regions,
		level_editor_grid
	>;

	// these register all the needed resources and console vars to use the 
	// level_editor defined above
	//NOTE: it is always safe to register or create the same resources, or console var twice
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
	// forces the new size to be a multiple of tile_size, or tile_size * n
	// 0 off
	// 1 on
	// n acts as if tiles size were multiplied by 'n'
	constexpr auto editor_level_force_whole_tiles = "editor_level_force_whole_tiles";
	constexpr auto editor_level_default_size = "editor_level_default_size";
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
}

#include "hades/detail/level_editor.inl"

#endif //!HADES_LEVEL_EDITOR_HPP
