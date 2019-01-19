#ifndef HADES_LEVEL_EDITOR_REGIONS_HPP
#define HADES_LEVEL_EDITOR_REGIONS_HPP

#include <array>
#include <vector>

#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Text.hpp"

#include "hades/collision.hpp"
#include "hades/font.hpp"
#include "hades/level.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/level_editor_grid.hpp"
#include "hades/math.hpp"
#include "hades/properties.hpp"

namespace hades
{
	void create_level_editor_regions_variables();

	class level_editor_regions final : public level_editor_component
	{
	public:
		struct region
		{
			sf::RectangleShape shape;
			sf::Text name;
		};

		using container_t = std::vector<region>;
		using index_t = container_t::size_type;

		struct region_edit
		{
			using array_t = std::array<sf::RectangleShape, 12u>;

			constexpr static auto nothing_selected = 
				std::numeric_limits<index_t>::max();
			index_t selected = nothing_selected;
			rect_corners held_corner{};
			direction held_edge{};
			string name;
			array_t selection_lines;
			vector_float drag_offset;
		};

		level_editor_regions();

		void level_load(const level&) override;
		level level_save(level l) const override;

		void gui_update(gui&, editor_windows&) override;

		void on_click(mouse_pos) override;

		void on_drag_start(mouse_pos) override;
		void on_drag(mouse_pos) override;
		void on_drag_end(mouse_pos) override;

		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override;
		void draw_brush_preview(sf::RenderTarget&, time_duration, sf::RenderStates) override;

	private:
		enum class brush_type {
			region_place,
			region_selector,
			region_move,
			region_edge_drag,
			region_corner_drag
		};

		//generates selection lines
		void _on_selected(index_t);
		//selects the correct drag point depending on the selection
		//line index
		void _on_drag_begin(region_edit::array_t::size_type);

		bool _show_regions = true;
		brush_type _brush = brush_type::region_place;
		console::property_float _region_min_size;
		container_t _regions;
		const resources::font *_font = nullptr;
		region_edit _edit;
		uint32 _new_region_name_counter{};
		grid_vars _grid = get_console_grid_vars();
		vector_t<level_size_t> _level_limits;
	};
}

namespace hades::cvars
{
	using namespace std::string_view_literals;
	constexpr auto region_min_size = "region_min_size"sv;
}

namespace hades::cvars::default_value
{
	constexpr auto region_min_size = 8.f;
}

#endif //!HADES_LEVEL_EDITOR_REGIONS_HPP
