#include "hades/level_editor_grid.hpp"

#include <cmath>

#include "hades/gui.hpp"
#include "hades/properties.hpp"

void hades::create_level_editor_grid_variables()
{
	console::create_property(cvars::editor_grid, cvars::default_value::editor_grid);
	console::create_property(cvars::editor_grid_auto, cvars::default_value::editor_grid_auto);
	console::create_property(cvars::editor_grid_snap, cvars::default_value::editor_grid_snap);
	console::create_property(cvars::editor_grid_size, cvars::default_value::editor_grid_size);
	console::create_property(cvars::editor_grid_step, cvars::default_value::editor_grid_step);
	console::create_property(cvars::editor_grid_step_max, cvars::default_value::editor_grid_step_max);
}

float hades::calculate_grid_size(float cell_size, int step) noexcept
{
	return cell_size * std::pow(2.f, static_cast<float>(step));
}

hades::int32 hades::calculate_grid_step_for_size(float cell_size, float target_size) noexcept
{
	const auto a = target_size / cell_size;
	const auto b = std::sqrt(a);
	const auto c = std::ceil(b);
	return static_cast<int32>(c) - 1;
}

hades::level_editor_grid::level_editor_grid() :
	_enabled{ console::get_bool(cvars::editor_grid, cvars::default_value::editor_grid) },
	_snap{ console::get_bool(cvars::editor_grid_snap, cvars::default_value::editor_grid_snap) },
	_size{ console::get_float(cvars::editor_grid_size, cvars::default_value::editor_grid_size) },
	_step{ console::get_int(cvars::editor_grid_step, cvars::default_value::editor_grid_step) },
	_step_max{ console::get_int(cvars::editor_grid_step_max, cvars::default_value::editor_grid_step_max) }
{
}

void hades::level_editor_grid::level_load(const level &l)
{
	_grid.set_all({ {static_cast<float>(l.map_x), static_cast<float>(l.map_y)}, calculate_grid_size(*_size, *_step), colours::white });
}

void hades::level_editor_grid::gui_update(gui &g, editor_windows &)
{
	using namespace std::string_view_literals;
	//add the grid toolbar buttons
	if (g.main_toolbar_begin())
	{
		if (g.toolbar_button("show/hide grid"sv))
			_enabled->store(!_enabled->load());
		if (g.toolbar_button("snap to grid"sv))
			_snap->store(!_snap->load());
		if (g.toolbar_button("grid grow"sv))
			_step->store(*_step + 1);
		if (g.toolbar_button("grid shrink"sv))
			_step->store(*_step - 1);
	}

	g.main_toolbar_end();

	//update the grid with any possible changes
	if (*_step > *_step_max)
		_step->store(*_step_max);
	else if (*_step < cvars::default_value::editor_grid_step)
		_step->store(cvars::default_value::editor_grid_step);

	const auto size = calculate_grid_size(*_size, *_step);
	_grid.set_cell_size(size);
}

void hades::level_editor_grid::draw(sf::RenderTarget &t, time_duration, sf::RenderStates s)
{
	if (*_enabled)
		t.draw(_grid, s);
}