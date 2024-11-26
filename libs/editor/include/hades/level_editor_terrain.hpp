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

	class level_editor_terrain final : public level_editor_component
	{
	public:
		enum class draw_shape : std::uint8_t {
			vertex [[deprecated]],
			edge [[deprecated]],
			rect,
			circle,
		};

		enum class brush_type : std::uint8_t {
			no_brush,
			// clear tile only
			erase,
			select_tile [[deprecated]],
			// art only
			draw_tile [[deprecated]],
			// ==terrain vertex==
			draw_terrain,
			// ==terrain height settings==
			raise_terrain,
			lower_terrain,
			plateau_terrain, // copies height from starting area
			add_height_noise, // create bumpiness
			smooth_height, // average out the different heights
			set_terrain_height [[deprecated]],
			// ==cliffs==
			raise_cliff, // add cliff
			lower_cliff,
			plataeu_cliff,
			add_ramp,
			remove_ramp,
			erase_cliff [[deprecated]],
			// ==water==
			raise_water,
			lower_water,
			// == unused==
			debug_brush [[deprecated]] // used for testing
		};

		level_editor_terrain();

		level level_new(level l) const override;
		void level_load(const level&) override;
		level level_save(level l) const override;
		void level_resize(vector2_int s, vector2_int o) override;

		void gui_update(gui&, editor_windows&) override;

		void make_brush_preview(time_duration, std::optional<terrain_target>) override;

		tag_list get_terrain_tags_at_location(rect_float) const override;

		void on_reinit(vector2_float window_size, vector2_float view_size) override;

		void on_click(std::optional<terrain_target>) override;
		void on_drag_start(std::optional<terrain_target>) override;
		void on_drag(std::optional<terrain_target>) override;
		//void on_drag_end(std::optional<terrain_target>) override;

		void on_screen_move(rect_float r) override;

		void on_height_toggle(bool) noexcept override;

		const terrain_map* peek_terrain() const noexcept override
		{
			return &_map.get_map();
		}

		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override;

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

		struct terrain_palette
		{
			std::string brush_name;
			std::string_view size_label = "Size: 1",
				shape_label = "Shape: Circle";
			brush_type brush = brush_type::no_brush;
			draw_shape shape = draw_shape::circle;
			uint8_t draw_size = 1;
			// stores the target drag level when drawing with height and cliff tools
			std::optional<uint8_t> drag_level;
		};

		void _gui_terrain_palette(gui&);
		bool _no_drag(std::optional<terrain_target>);

		console::property_int _view_height;

		resources::tile _empty_tile [[deprecated]];
		const resources::terrain * _empty_terrain [[deprecated]];
		const resources::terrainset* _empty_terrainset [[deprecated]];
		resources::tile_size_t _tile_size;

		//brush settings
		terrain_palette _terrain_palette;
		[[deprecated]] int _size = 1;
		std::uint8_t _height_strength = 1;
		std::uint8_t _set_height = 1; // TODO:
		[[deprecated]] std::uint8_t _cliff_default_height = 5;

		//selected tile/terrain
		[[deprecated]] resources::tile _tile = resources::bad_tile;
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
	constexpr auto editor_default_terrainset = "editor_default_terrainset";

	namespace default_value
	{
		constexpr auto editor_default_terrainset = "";
	}
}

#endif //!HADES_LEVEL_EDITOR_TERRAIN_HPP
