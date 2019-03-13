#ifndef HADES_LEVEL_EDITOR_TERRAIN_HPP
#define HADES_LEVEL_EDITOR_TERRAIN_HPP

#include "hades/data.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/terrain_map.hpp"

namespace hades
{
	void register_level_editor_terrain_resources(data::data_manager&);

	class level_editor_terrain final : public level_editor_component
	{
	public:
		level_editor_terrain();

		level level_new(level l) const override;
		void level_load(const level&) override;
		//level level_save(level l) const override;

		void gui_update(gui&, editor_windows&) override;

		//void make_brush_preview(time_duration, mouse_pos) override;

		//void on_click(mouse_pos) override;
		//void on_drag(mouse_pos) override;

		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override;
		//void draw_brush_preview(sf::RenderTarget&, time_duration, sf::RenderStates) override;

	private:
		enum class brush_type {
			no_brush,
			erase,
			draw_tile,
			draw_terrain
		};

		enum class draw_shape {
			rect,
			circle
		};

		struct level_options
		{
			const resources::terrainset *terrain_set = nullptr;
			const resources::terrain *terrain = nullptr;
		};

		resources::tile _empty_tile = resources::get_empty_tile();
		const resources::terrain * _empty_terrain = resources::get_empty_terrain();
		resources::tile_size_t _tile_size = resources::get_tile_settings()->tile_size;

		//brush settings
		brush_type _brush = brush_type::no_brush;
		draw_shape _shape = draw_shape::rect;
		int _size = 0;

		//selected tile/terrain
		resources::tile _tile = resources::bad_tile;
		level_options _current;

		const resources::terrain_settings *_settings = nullptr;
		level_options _new_options;
		mutable_terrain_map _map;
	};
}

#endif //!HADES_LEVEL_EDITOR_TERRAIN_HPP
