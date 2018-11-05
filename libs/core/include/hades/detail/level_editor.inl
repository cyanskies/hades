#include "hades/level_editor.hpp"

#include <tuple>

#include "hades/animation.hpp"
#include "hades/properties.hpp"
#include "hades/console_variables.hpp"
#include "hades/for_each_tuple.hpp"

namespace hades
{
	template<typename Func, typename ...Components>
	inline void level_editor_do_tuple_work(std::tuple<Components...> &t, std::size_t i, Func f)
	{
		//if no brush is set, then do nothing
		if (i > std::tuple_size_v<std::tuple<Components...>>)
			return;

		for_index_tuple(t, i, f);
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_click(brush_index_t i, vector_float p)
	{
		level_editor_do_tuple_work(_editor_components, i, [p](auto &&c) {
			c.on_click(p);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_drag_start(brush_index_t i, vector_float p)
	{
		level_editor_do_tuple_work(_editor_components, i, [p](auto &&c) {
			c.on_drag_start(p);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_drag(brush_index_t i, vector_float p)
	{
		level_editor_do_tuple_work(_editor_components, i, [p](auto &&c) {
			c.on_drag(p);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_drag_end(brush_index_t i, vector_float p)
	{
		level_editor_do_tuple_work(_editor_components, i, [p](auto &&c) {
			c.on_drag_end(p);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_draw_components(sf::RenderTarget &target, time_duration delta_time)
	{
		auto states = sf::RenderStates{};
		for_each_tuple(_editor_components, [&target, &delta_time, states](auto &&v) {
			v.draw(target, delta_time, states);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_generate_brush_preview(brush_index_t brush_index, vector_float world_position)
	{
		level_editor_do_tuple_work(_editor_components, brush_index, [world_position](auto &&c) {
			c.make_brush_preview(world_position);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_handle_component_setup()
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