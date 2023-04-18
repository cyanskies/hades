#ifndef HADES_LEVEL_EDITOR_TERRAIN_HPP
#define HADES_LEVEL_EDITOR_TERRAIN_HPP

#include "hades/data.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/terrain_map.hpp"

namespace hades
{
	void register_level_editor_terrain_resources(data::data_manager&);
	void create_level_editor_terrain_variables();

	class level_editor_terrain final : public level_editor_component
	{
	public:
		enum class draw_shape {
			rect,
			circle
		};

		level_editor_terrain();

		level level_new(level l) const override;
		void level_load(const level&) override;
		level level_save(level l) const override;
		void level_resize(vector_int s, vector_int o) override;

		void gui_update(gui&, editor_windows&) override;

		void make_brush_preview(time_duration, mouse_pos) override;

		tag_list get_terrain_tags_at_location(rect_float) const override;

		void on_click(mouse_pos) override;
		void on_drag(mouse_pos) override;

		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override;
		void draw_brush_preview(sf::RenderTarget&, time_duration, sf::RenderStates) override;

	private:
		enum class brush_type {
			no_brush,
			erase,
			draw_tile,
			draw_terrain
		};

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

		resources::tile _empty_tile;
		const resources::terrain * _empty_terrain;
		const resources::terrainset* _empty_terrainset;
		resources::tile_size_t _tile_size;

		//brush settings
		brush_type _brush = brush_type::no_brush;
		draw_shape _shape = draw_shape::rect;
		int _size = 0;

		//brush preview
		mutable_terrain_map _clear_preview;
		mutable_terrain_map _preview;

		//selected tile/terrain
		resources::tile _tile = resources::bad_tile;
		level_options _current;
		level_resize_options _resize;

		const resources::terrain_settings *_settings = nullptr;
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
