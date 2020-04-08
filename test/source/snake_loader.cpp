#include "snake_loader.hpp"

//#include "yaml-cpp/yaml.h"

#include "Hades/Logging.hpp"

snake_rules *snake_game_rules = nullptr;

void parseSnakeRules(hades::unique_id mod,/*YAML::Node& node,*/ hades::data::data_manager* dataman)
{
	//default snake yaml
	//snake-game:
	//    start-length: 
	//	  snake-speed:

	//snake_rules *s;
	//auto id = dataman->get_uid("snake-game");

	//try
	//{
	//	s = dataman->get<snake_rules>(id);
	//}
	//catch (hades::data::resource_null&)
	//{
	//	//resource doens't exist yet, create it
	//	auto snake_ptr = std::make_unique<snake_rules>();
	//	s = &*snake_ptr;
	//	dataman->set<snake_rules>(id, std::move(snake_ptr));

	//	s->snake_length = 1;
	//	s->snake_speed_ms = 1;
	//}
	//catch (hades::data::resource_wrong_type&)
	//{
	//	//name is already used for something else, this cannnot be loaded
	//	auto mod_ptr = dataman->get<hades::resources::mod>(mod);
	//	LOGERROR("Name collision with identifier: 'snake-game', for texture while parsing mod: " + mod_ptr->name + ". Name has already been used for a different resource type.");
	//	//this is probably unrecoverable for game settings objects
	//	return;
	//}

	//auto slength = node["snake-length"];
	//if (!slength.IsNull())
	//	s->snake_length = slength.as<hades::types::uint32>(s->snake_length);
	//
	//auto sspeed = node["snake-speed-ms"];
	//if (!sspeed.IsNull())
	//	s->snake_speed_ms = sspeed.as<hades::types::uint32>(s->snake_speed_ms);
}