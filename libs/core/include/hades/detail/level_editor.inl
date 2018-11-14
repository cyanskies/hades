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
	inline void basic_level_editor<Components...>::_component_on_load(const level &l)
	{
		for_each_tuple(_editor_components, [&l](auto &&c) {
			c.level_load(l);
		});
	}

	template<typename ...Components>
	inline level basic_level_editor<Components...>::_component_on_save(level l) const
	{
		for_each_tuple(_editor_components, [&l](auto &&c) {
			l = c.level_save(std::move(l));
		});

		return l;
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
	inline void basic_level_editor<Components...>::_draw_components(sf::RenderTarget &target, time_duration delta_time, brush_index_t active_brush)
	{
		for_each_tuple(_editor_components, [&target, &delta_time](auto &&v) {
			v.draw(target, delta_time, sf::RenderStates{});
		});

		//draw brush preview
		if (active_brush == invalid_brush)
			return;

		for_index_tuple(_editor_components, active_brush, [&target, &delta_time](auto &&v) {
			v.draw_brush_preview(target, delta_time, sf::RenderStates{});
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_generate_brush_preview(brush_index_t brush_index, time_duration dt, vector_float world_position)
	{
		level_editor_do_tuple_work(_editor_components, brush_index, [dt, world_position](auto &&c) {
			c.make_brush_preview(dt, world_position);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_handle_component_setup()
	{
		_editor_components = component_tuple{};

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
	inline void basic_level_editor<Components...>::_update_component_gui(gui &g, level_editor_component::editor_windows &win_flags)
	{
		for_each_tuple(_editor_components, [&g, &win_flags](auto &&c) {
			c.gui_update(g, win_flags);
		});
	}
}