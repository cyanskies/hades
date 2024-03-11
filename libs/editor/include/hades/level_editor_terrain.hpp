#ifndef HADES_LEVEL_EDITOR_TERRAIN_HPP
#define HADES_LEVEL_EDITOR_TERRAIN_HPP

#include "hades/data.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/properties.hpp"
#include "hades/terrain_map.hpp"

namespace hades
{
	void register_level_editor_terrain_resources(data::data_manager&);
	void create_level_editor_terrain_variables();

	class tile_mutator final
	{
	public:
		void select_tile(const tile_position p) noexcept
		{
			_tile = p;
			return;
		}

		tile_position current_tile() const noexcept
		{
			return _tile;
		}

		// returns true to activate tile selection mode
		bool update_gui(gui&, mutable_terrain_map& map, mutable_terrain_map& preview);

		void open() noexcept
		{
			_open = true;
			return;
		}

	private:
		tile_position _tile = bad_tile_position;
		std::uint8_t _set_height = {};
		bool _open = false;
	};

	class level_editor_terrain final : public level_editor_component
	{
	public:
		enum class draw_shape : std::uint8_t {
			vertex,
			edge,
			rect,
			circle,
		};

		enum class brush_type : std::uint8_t {
			no_brush,
			// clear tile only
			erase,
			select_tile,
			// art only
			draw_tile,
			// terrain vertex
			draw_terrain,
			// terrain height
			raise_terrain,
			lower_terrain,
			set_terrain_height,
			// cliffs
			raise_cliff, // add cliff
			//lower_cliff,
			erase_cliff,
			debug_brush // used for testing
		};

		level_editor_terrain();

		level level_new(level l) const override;
		void level_load(const level&) override;
		level level_save(level l) const override;
		void level_resize(vector2_int s, vector2_int o) override;

		void gui_update(gui&, editor_windows&) override;

		void make_brush_preview(time_duration, mouse_pos) override;

		tag_list get_terrain_tags_at_location(rect_float) const override;

		void on_reinit(vector2_float window_size, vector2_float view_size) override;

		void on_click(mouse_pos) override;
		void on_drag(mouse_pos) override;

		void on_screen_move(rect_float r) override;

		void on_height_toggle(bool) noexcept override;

		const terrain_map* peek_terrain() const noexcept override
		{
			return &_map.get_map();
		}

		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override;
		void draw_brush_preview(sf::RenderTarget&, time_duration, sf::RenderStates) override;

	private:
		struct level_options
		{
			const resources::terrainset *terrain_set = nullptr;
			// current terrain brush
			const resources::terrain *terrain = nullptr;
		};

		struct level_resize_options
		{
			const resources::terrain* terrain = nullptr;
		};

		console::property_int _view_height;

		tile_mutator _tile_mutator;
		resources::tile _empty_tile;
		const resources::terrain * _empty_terrain;
		const resources::terrainset* _empty_terrainset;
		resources::tile_size_t _tile_size;

		//brush settings
		brush_type _brush = brush_type::no_brush;
		draw_shape _shape = draw_shape::rect;
		int _size = 1;
		std::uint8_t _height_strength = 1;
		std::uint8_t _set_height = 1; // TODO:
		std::uint8_t _cliff_default_height;

		//brush preview
		mutable_terrain_map _clear_preview;
		mutable_terrain_map _preview;

		//selected tile/terrain
		resources::tile _tile = resources::bad_tile;
		level_options _current;
		level_resize_options _resize;

		const resources::terrain_settings *_settings = nullptr;
		float _sun_angle = 135.f; // TODO: default sun angle cvar
		level_options _new_options;
		mutable_terrain_map _map;
	};
}

namespace hades::cvars
{
	constexpr auto editor_default_terrainset = "editor_default_terrinset";

	namespace default_value
	{
		constexpr auto editor_default_terrainset = "";
	}
}

#endif //!HADES_LEVEL_EDITOR_TERRAIN_HPP
