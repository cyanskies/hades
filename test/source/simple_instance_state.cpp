#include "simple_instance_state.hpp"

#include "hades/console_variables.hpp"
#include "Hades/game_system.hpp"
#include "hades/files.hpp"
#include "hades/level.hpp"
#include "hades/Main.hpp"
#include "hades/properties.hpp"

using namespace std::string_view_literals;

void simple_on_connect(hades::job_system&, hades::render_job_data &d)
{
	const auto entity = d.entity;
}

void simple_on_tick(hades::job_system&, hades::render_job_data &d)
{
	const auto entity = d.entity;
}

void simple_on_disconnect(hades::job_system&, hades::render_job_data &d)
{
	const auto entity = d.entity;
}

void register_simple_instance_resources(hades::data::data_manager &d)
{
	hades::register_game_server_resources(d);

	hades::make_render_system("simple_render"sv,
		nullptr,
		simple_on_connect,
		simple_on_disconnect,
		simple_on_tick,
		nullptr,
		d
	);
}

void simple_instance_state::init()
{
	const auto lvl_str = hades::files::as_string(defaultGame(), "new_level.lvl"sv);
	const auto level = hades::deserialise(lvl_str);
	const auto lvl_sv = hades::make_save_from_level(level);

	_server = hades::create_server(lvl_sv);
	_level = _server->connect_to_level(hades::unique_id::zero);

	const auto level_state = _level->get_changes();
	//_client_instance.input_updates(level_state);
}

bool simple_instance_state::handle_event(const hades::event & windowEvent)
{
	return false;
}

void simple_instance_state::update(hades::time_duration dt, const sf::RenderTarget &t, hades::input_system::action_set a)
{
	_server->update(dt);
}

void simple_instance_state::draw(sf::RenderTarget &target, hades::time_duration deltaTime)
{
	const auto changes = _level->get_changes();
	//_client_instance.input_updates(changes);
	//_client_instance.
}

void simple_instance_state::reinit()
{
	const auto width = hades::console::get_int(hades::cvars::video_width);
	const auto height = hades::console::get_int(hades::cvars::video_height);

	_view.setSize({ static_cast<float>(width->load()), static_cast<float>(height->load()) });
}

void simple_instance_state::pause()
{
}

void simple_instance_state::resume()
{
}
