#ifndef HADES_LEVEL_EDITOR_HPP
#define HADES_LEVEL_EDITOR_HPP

#include <string_view>
#include <tuple>

#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/gui.hpp"
#include "hades/level.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/properties.hpp"
#include "hades/mouse_input.hpp"
#include "hades/state.hpp"
#include "hades/types.hpp"
#include "hades/vector_math.hpp"

namespace hades::detail
{
	constexpr auto mouse_drag_enabled = true;
	constexpr auto mouse_double_click_enabled = false;

	class level_editor_impl : public state
	{
	public:
		level_editor_impl();
		level_editor_impl(level);

		virtual ~level_editor_impl() noexcept = default;

		void init() override;
		bool handle_event(const event&) override;
		void reinit() override;

		void update(time_duration, const sf::RenderTarget&, input_system::action_set) override;

		void draw(sf::RenderTarget&, time_duration) override;

	protected:
		using brush_index_t = std::size_t;

		virtual void _component_on_load(const level&) = 0;
		virtual level _component_on_save(level) const = 0;
		virtual void _component_on_click(brush_index_t, vector_float) = 0;
		virtual void _component_on_drag_start(brush_index_t, vector_float) = 0;
		virtual void _component_on_drag(brush_index_t, vector_float) = 0;
		virtual void _component_on_drag_end(brush_index_t, vector_float) = 0;
		virtual void _draw_components(sf::RenderTarget&, time_duration, brush_index_t) = 0;
		virtual void _generate_brush_preview(brush_index_t brush_index, time_duration, vector_float world_position) = 0;
		virtual void _handle_component_setup() = 0;
		void _set_active_brush(std::size_t index);
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
		//level_size_t _level_x = 0, _level_y = 0;

		sf::View _gui_view;
		sf::View _world_view;

	private:
		struct new_level_opt
		{
			int32 width{}, height{};
		};

		void _update_gui(time_duration);

		level_editor_component::editor_windows _window_flags;
		gui _gui;
		level _level;
		mouse::mouse_button_state<mouse_drag_enabled, mouse_double_click_enabled> _mouse_left;
		new_level_opt _new_level_options;

		sf::RectangleShape _background;

		time_point _total_run_time;

		//currently active brush
		constexpr static auto invalid_brush = std::numeric_limits<brush_index_t>::max();
		brush_index_t _active_brush = invalid_brush;
	};
}

namespace hades
{
	void create_editor_console_variables();

	template<typename ...Components>
	class basic_level_editor final : public detail::level_editor_impl
	{
	private:
		void _component_on_load(const level&) override;
		level _component_on_save(level) const override;
		tag_list _component_get_tags_at_location(rect_float) const;
		void _component_on_click(brush_index_t, vector_float) override;
		void _component_on_drag_start(brush_index_t, vector_float) override;
		void _component_on_drag(brush_index_t, vector_float) override;
		void _component_on_drag_end(brush_index_t, vector_float) override;
		void _draw_components(sf::RenderTarget&, time_duration, brush_index_t) override;
		void _generate_brush_preview(brush_index_t, time_duration, vector_float) override;
		void _handle_component_setup() override;
		void _update_component_gui(gui&, level_editor_component::editor_windows&) override;

		using component_tuple = std::tuple<Components...>;
		component_tuple _editor_components;
	};

	using level_editor = basic_level_editor<>;
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

	constexpr auto editor_level_default_size = 100;
}

#include "hades/detail/level_editor.inl"

#endif //!HADES_LEVEL_EDITOR_HPP
