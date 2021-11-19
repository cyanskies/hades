#include "hades/level_scripts.hpp"

#include <vector>

#include "hades/data.hpp"

using namespace std::string_view_literals;
//constexpr auto player_script_name = "player-input-scripts"sv;

static std::vector<hades::unique_id> player_input_resources{};
static std::vector<hades::unique_id> level_load_scripts{};

//static void parse_player_script(hades::unique_id, const hades::data::parser_node&, hades::data::data_manager&)
//{
//	return;
//}

namespace hades::resources
{
	void hades::resources::make_player_input_resource(std::string_view n, player_input::player_input_fn f, data::data_manager &d)
	{
		assert(f);
		const auto id = d.get_uid(n);
		auto p_i = d.find_or_create<player_input>(id, unique_id::zero);
		assert(p_i);
		p_i->function = std::move(f);
		p_i->loaded = true;
		player_input_resources.emplace_back(id);
		return;
	}

	const std::vector<unique_id>& get_player_input_list() noexcept
	{
		return player_input_resources;
	}

	void hades::resources::make_level_load_script_resource(std::string_view n, load_script::load_fn f, data::data_manager& d)
	{
		assert(f);
		const auto id = d.get_uid(n);
		auto l_i = d.find_or_create<load_script>(id, unique_id::zero);
		assert(l_i);
		l_i->function = std::move(f);
		l_i->loaded = true;
		level_load_scripts.emplace_back(id);
		return;
	}

	const std::vector<unique_id>& get_level_load_script_list() noexcept
	{
		return level_load_scripts;
	}
}

namespace hades
{
	void register_level_scripts_resource(data::data_manager &d)
	{
		//player input scripts
		//d.register_resource_type(player_script_name, parse_player_script);
		//d.register_resource_type(load_script_name, parse_load_script);
		return;
	}
}
