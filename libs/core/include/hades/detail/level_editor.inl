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
	inline level basic_level_editor<Components...>::_component_on_new(level l) const
	{
		for_each_tuple(_editor_components, [&l](auto &&c) {
			l = c.level_new(std::move(l));
		});

		return l;
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
	inline void basic_level_editor<Components...>::_component_on_resize(vector_int size, vector_int offset)
	{
		for_each_tuple(_editor_components, [size, offset](auto&& c) {
			c.level_resize(size, offset);
			return;
		});
		return;
	}

	template<typename ...Components>
	inline tag_list basic_level_editor<Components...>::_component_get_object_tags_at_location(rect_float area) const
	{
		auto func = [](auto &&c, rect_float rect)->tag_list {
			return c.get_object_tags_at_location(rect);
		};

		const auto results = for_each_tuple_r(_editor_components, func, area);

		auto size = std::size_t{};
		for (const auto& r : results)
			size += std::size(r);

		auto output = tag_list{};
		output.reserve(size);
		for (const auto& r : results)
			output.insert(std::end(output), std::begin(r), std::end(r));

		return output;
	}

	template<typename ...Components>
	inline tag_list basic_level_editor<Components...>::_component_get_terrain_tags_at_location(rect_float area) const
	{
		auto func = [](auto&& c, rect_float rect)->tag_list {
			return c.get_terrain_tags_at_location(rect);
		};

		const auto results = for_each_tuple_r(_editor_components, func, area);

		auto size = std::size_t{};
		for (const auto& r : results)
			size += std::size(r);

		auto output = tag_list{};
		for (const auto& r : results)
			output.insert(std::end(output), std::begin(r), std::end(r));

		return output;
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
		for_each_tuple(_editor_components, [this](auto &&c, std::size_t index) {
			auto activate_brush = [this, index] () noexcept {
				_set_active_brush(index);
			};

			auto get_object_tags_at = [this](rect_float area)->tag_list {
				return _component_get_object_tags_at_location(area);
			};

			auto get_terrain_tags_at = [this](rect_float area)->tag_list {
				return _component_get_terrain_tags_at_location(area);
			};

			auto get_players_fn = [this]() {
				return get_players();
			};

			c.install_callbacks(
				activate_brush,
				get_terrain_tags_at,
				get_object_tags_at,
				get_players_fn
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