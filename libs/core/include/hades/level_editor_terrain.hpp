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
		level level_new(level l) const override;
		//void level_load(const level&) override;
		//level level_save(level l) const override;

		void gui_update(gui&, editor_windows&) override;

		//void make_brush_preview(time_duration, mouse_pos) override;

		//void on_click(mouse_pos) override;
		//void on_drag(mouse_pos) override;

		//void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override;
		//void draw_brush_preview(sf::RenderTarget&, time_duration, sf::RenderStates) override;

	private:
		struct level_options
		{
			const resources::terrainset *terrain_set = nullptr;
			const resources::terrain *terrain = nullptr;
		};

		level_options _new_options;
		mutable_terrain_map _map;
	};
}

#endif //!HADES_LEVEL_EDITOR_TERRAIN_HPP
