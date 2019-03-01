#ifndef HADES_LEVEL_EDITOR_TILES_HPP
#define HADES_LEVEL_EDITOR_TILES_HPP

#include "hades/data.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/tile_map.hpp"

namespace hades
{
	void register_level_editor_tiles_resources(data::data_manager&);

	class level_editor_tiles : public level_editor_component
	{
	public:
		enum class draw_shape {
			square,
			circle
		};

		level level_new(level l) const override;
		void level_load(const level&) override;
		level level_save(level l) const override;

		void gui_update(gui&, editor_windows&) override;

		void make_brush_preview(time_duration, mouse_pos) override;

		void on_click(mouse_pos) override;
		void on_drag(mouse_pos) override;

		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override;
		void draw_brush_preview(sf::RenderTarget&, time_duration, sf::RenderStates) override;

	private:
		struct new_level_options
		{
			const resources::tileset *tileset = nullptr;
			const resources::tile *tile = nullptr;
		};

		new_level_options _new_options{};
		vector_t<level_size_t> _level_size;
		draw_shape _shape{ draw_shape::square };
		int _size{};
		const resources::tile_settings *_settings{ resources::get_tile_settings() };
		const resources::tile *_tile = nullptr;
		mutable_tile_map _tiles;
		mutable_tile_map _empty_preview;
		mutable_tile_map _preview;
	};
}

#endif //!HADES_LEVEL_EDITOR_TILES_HPP
