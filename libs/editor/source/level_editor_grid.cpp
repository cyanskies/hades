#include "hades/level_editor_grid.hpp"

#include <cmath>

#include "hades/gui.hpp"
#include "hades/properties.hpp"

void hades::create_level_editor_grid_variables()
{
	console::create_property(cvars::editor_grid, cvars::default_value::editor_grid);
	console::create_property(cvars::editor_grid_auto, cvars::default_value::editor_grid_auto);
	console::create_property(cvars::editor_grid_snap, cvars::default_value::editor_grid_snap);
	console::create_property(cvars::editor_grid_step_min, cvars::default_value::editor_grid_step_min);
	console::create_property(cvars::editor_grid_step, cvars::default_value::editor_grid_step);
	console::create_property(cvars::editor_grid_step_max, cvars::default_value::editor_grid_step_max);
}

float hades::calculate_grid_size(int32 step) noexcept
{
	return std::pow(2.f, static_cast<float>(step));
}

float hades::calculate_grid_size(const grid_vars &g)
{
	return calculate_grid_size(g.step->load());
}

hades::int32 hades::calculate_grid_step_for_size(float target_size) noexcept
{
	return static_cast<int32>(std::log2(target_size));
}

hades::grid_vars hades::get_console_grid_vars()
{
	return grid_vars{ 
		console::get_bool(cvars::editor_grid_auto, cvars::default_value::editor_grid_auto),
		console::get_bool(cvars::editor_grid, cvars::default_value::editor_grid),
		console::get_bool(cvars::editor_grid_snap, cvars::default_value::editor_grid_snap),
		console::get_int(cvars::editor_grid_step_min, cvars::default_value::editor_grid_step_min),
		console::get_int(cvars::editor_grid_step, cvars::default_value::editor_grid_step),
		console::get_int(cvars::editor_grid_step_max, cvars::default_value::editor_grid_step_max) 
	};
}

void hades::level_editor_grid::level_load(const level &l)
{
	_grid.set_all({ {static_cast<float>(l.map_x), static_cast<float>(l.map_y)},
		calculate_grid_size(*_grid_vars.step), colours::from_name(colours::names::white) });
}

void hades::level_editor_grid::level_resize(vector2_int s, vector2_int)
{
	_grid.set_size({static_cast<float>(s.x), static_cast<float>(s.y)});
}

void hades::level_editor_grid::gui_update(gui &g, editor_windows &)
{
	using namespace std::string_view_literals;
	//add the grid toolbar buttons
	if (g.main_toolbar_begin())
	{
		if (g.toolbar_button("show/hide grid"sv))
			_grid_vars.enabled->store(!_grid_vars.enabled->load());
		if (g.toolbar_button("snap to grid"sv))
			_grid_vars.snap->store(!_grid_vars.snap->load());
		if (g.toolbar_button("grid grow"sv))
			_grid_vars.step->store(*_grid_vars.step + 1);
		if (g.toolbar_button("grid shrink"sv))
			_grid_vars.step->store(*_grid_vars.step - 1);
	}

	g.main_toolbar_end();

	//update the grid with any possible changes
	if (*_grid_vars.step > *_grid_vars.step_max)
		_grid_vars.step->store(*_grid_vars.step_max);
	else if (*_grid_vars.step < cvars::default_value::editor_grid_step)
		_grid_vars.step->store(cvars::default_value::editor_grid_step);

	const auto size = calculate_grid_size(*_grid_vars.step);
	if (size != _previous_size)
	{
		_previous_size = size;
		_grid.set_cell_size(size);
	}
}

void hades::level_editor_grid::draw(sf::RenderTarget &t, time_duration, sf::RenderStates s)
{
	if (*_grid_vars.enabled)
		t.draw(_grid, s);
}
