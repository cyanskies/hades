#include "hades/level_editor.hpp"

#include <tuple>

#include "hades/animation.hpp"
#include "hades/properties.hpp"
#include "hades/console_variables.hpp"
#include "hades/tuple.hpp"

namespace hades
{
	template<tuple Components, typename Func>
	inline void level_editor_do_tuple_work(Components&& t, std::size_t i, Func&& f)
	{
		//if no brush is set, then do nothing
		if (i > std::tuple_size_v<std::remove_reference_t<Components>>)
			return;

		tuple_index_invoke(std::forward<Components>(t), i, std::forward<Func>(f));
	}

	template<typename ...Components>
	inline level basic_level_editor<Components...>::_component_on_new(level l) const
	{
		tuple_for_each(_editor_components, [&l](auto &&c) {
			l = c.level_new(std::move(l));
		});

		return l;
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_load(const level &l)
	{
		tuple_for_each(_editor_components, [&l](auto &&c) {
			c.level_load(l);
		});
	}

	template<typename ...Components>
	inline level basic_level_editor<Components...>::_component_on_save(level l) const
	{
		tuple_for_each(_editor_components, [&l](auto &&c) {
			l = c.level_save(std::move(l));
		});

		return l;
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_resize(vector2_int size, vector2_int offset)
	{
		tuple_for_each(_editor_components, [size, offset](auto&& c) {
			c.level_resize(size, offset);
			return;
		});
		return;
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_reinit(vector2_float window, vector2_float view)
	{
		tuple_for_each(_editor_components, [window, view](auto&& c) {
			c.on_reinit(window, view);
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

		const auto results = tuple_transform(_editor_components, func, area);

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

		const auto results = tuple_transform(_editor_components, func, area);

		auto size = std::size_t{};
		for (const auto& r : results)
			size += std::size(r);

		auto output = tag_list{};
		for (const auto& r : results)
			output.insert(std::end(output), std::begin(r), std::end(r));

		return output;
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_click(brush_index_t i, vector2_float p)
	{
		level_editor_do_tuple_work(_editor_components, i, [p](auto &&c) {
			c.on_click(p);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_drag_start(brush_index_t i, vector2_float p)
	{
		level_editor_do_tuple_work(_editor_components, i, [p](auto &&c) {
			c.on_drag_start(p);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_drag(brush_index_t i, vector2_float p)
	{
		level_editor_do_tuple_work(_editor_components, i, [p](auto &&c) {
			c.on_drag(p);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_drag_end(brush_index_t i, vector2_float p)
	{
		level_editor_do_tuple_work(_editor_components, i, [p](auto &&c) {
			c.on_drag_end(p);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_rotate(float f)
	{
		tuple_for_each(_editor_components, [f](auto &&v) {
			v.on_world_rotate(f);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_on_screen_move(rect_float r)
	{
		tuple_for_each(_editor_components, [r](auto&& v) {
			v.on_screen_move(r);
			});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_component_height_toggle(bool b)
	{
		tuple_for_each(_editor_components, &level_editor_component::on_height_toggle, b);
	}

	template<typename ...Components>
	const terrain_map* basic_level_editor<Components...>::_component_peek_terrain()
	{
		const auto terrains = tuple_transform(_editor_components, &level_editor_component::peek_terrain);
		for (const auto t : terrains)
		{
			if (t)
				return t;
		}
		
		return {};
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_draw_components(sf::RenderTarget &target,
		time_duration delta_time, brush_index_t active_brush, sf::RenderStates s)
	{
		tuple_for_each(_editor_components, [&target, &delta_time, &s](auto &&v) {
			v.draw(target, delta_time, s);
		});

		//draw brush preview
		if (active_brush == invalid_brush)
			return;

		tuple_index_invoke(_editor_components, active_brush, [&target, &delta_time, &s](auto &&v) {
			v.draw_brush_preview(target, delta_time, s);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_generate_brush_preview(brush_index_t brush_index, time_duration dt, vector2_float world_position)
	{
		level_editor_do_tuple_work(_editor_components, brush_index, [dt, world_position](auto &&c) {
			c.make_brush_preview(dt, world_position);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_handle_component_setup()
	{
		tuple_for_each(_editor_components, [this](auto &&c, std::size_t index) {
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

			auto get_world_rotation_fn = [this]() {
				return _get_world_rot();
			};

			c.install_callbacks(
				activate_brush,
				get_terrain_tags_at,
				get_object_tags_at,
				get_players_fn,
				get_world_rotation_fn
			);
		});
	}

	template<typename ...Components>
	inline void basic_level_editor<Components...>::_update_component_gui(gui &g, level_editor_component::editor_windows &win_flags)
	{
		tuple_for_each(_editor_components, [&g, &win_flags](auto &&c) {
			c.gui_update(g, win_flags);
		});
	}
}