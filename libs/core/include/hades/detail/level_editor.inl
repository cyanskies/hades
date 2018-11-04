#include "hades/level_editor.hpp"

#include "hades/animation.hpp"
#include "hades/properties.hpp"
#include "hades/console_variables.hpp"
#include "hades/for_each_tuple.hpp"

namespace hades
{
	template<typename ...Components>
	inline void basic_level_editor<Components...>::_draw_components(sf::RenderTarget &target, time_duration delta_time)
	{
		auto states = sf::RenderStates{};
		for_each_tuple(_editor_components, [&target, &delta_time, states](auto &&v) {
			v.draw(target, delta_time, states);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_generate_brush_preview(std::size_t brush_index, vector_float world_position)
	{
		for_index_tuple(_editor_components, brush_index, [world_position](auto &&c) {
			c.make_brush_preview(world_position);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_hand_component_setup()
	{
		for_each_tuple(_editor_components, [this](auto &&c, std::size_t index) {
			auto activate_brush = [this, index] {
				_set_active_brush(index);
			};

			c.install_callbacks(
				activate_brush
			);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_update_component_gui(gui &g)
	{
		for_each_tuple(_editor_components, [&g](auto &&c) {
			c.gui_update(g);
		});
	}
}