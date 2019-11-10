#ifndef HADES_LEVEL_EDITOR_GRID_HPP
#define HADES_LEVEL_EDITOR_GRID_HPP

#include<string_view>

#include "hades/grid.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/properties.hpp"

namespace hades
{
	void create_level_editor_grid_variables();

	struct grid_vars
	{
		console::property_bool auto_mode;
		console::property_bool enabled;
		console::property_bool snap;
		console::property_float size;
		console::property_int step;
		console::property_int step_max;
	};

	grid_vars get_console_grid_vars();

	float calculate_grid_size(float cell_size, int step) noexcept;
	float calculate_grid_size(const grid_vars&);
	int32 calculate_grid_step_for_size(float cell_size, float target_size) noexcept;
	int32 calculate_grid_step_for_size(const grid_vars&, float target_size);

	template<typename T>
	vector_t<T> snap_to_grid(vector_t<T> p, const grid_vars&);

	class level_editor_grid final : public level_editor_component
	{
	public:
		void level_load(const level&) override;
		void gui_update(gui&, editor_windows&) override;

		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override;
	private:
		grid _grid;
		float _previous_size{};
		grid_vars _grid_vars = get_console_grid_vars();
	};
}

namespace hades::cvars
{
	using namespace std::string_view_literals;
	constexpr auto editor_grid = "editor_grid"sv;
	// allows the editor to change grid settings automatically depending on current tool
	constexpr auto editor_grid_auto = "editor_grid_auto"sv;
	// toggle whether tools should snap to the nearest grid slot
	constexpr auto editor_grid_snap = "editor_grid_snap"sv;
	// the distance between lines at grid step 0
	constexpr auto editor_grid_size = "editor_grid_size"sv;
	// the maximum step level
	constexpr auto editor_grid_step_max = "editor_grid_step_max"sv;
	// the current step level, 
	// distance between lines is doubled for every step level
	constexpr auto editor_grid_step = "editor_grid_step"sv;
}

namespace hades::cvars::default_value
{
	constexpr auto editor_grid = true;
	constexpr auto editor_grid_auto = true;
	constexpr auto editor_grid_snap = true;
	constexpr auto editor_grid_size = 4.f;
	constexpr auto editor_grid_step_max = 5;
	constexpr auto editor_grid_step = 1;
}

#include "hades/detail/level_editor_grid.inl"

#endif //!HADES_LEVEL_EDITOR_GRID_HPP
