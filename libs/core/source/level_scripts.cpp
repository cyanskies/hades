#include "hades/level_scripts.hpp"

#include <vector>

#include "hades/data.hpp"

using namespace std::string_view_literals;
constexpr auto player_script_name = "player-input-scripts"sv;

static std::vector<hades::unique_id> player_input_resources{};

static void parse_player_script(hades::unique_id, const hades::data::parser_node&, hades::data::data_manager&)
{
	return;
}

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
}

namespace hades
{
	void register_level_scripts_resource(data::data_manager &d)
	{
		//player input scripts
		d.register_resource_type(player_script_name, parse_player_script);
		return;
	}
}
