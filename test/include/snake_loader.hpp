#ifndef TEST_SNAKE_LOADER_HPP
#define TEST_SNAKE_LOADER_HPP

#include "Hades/data_manager.hpp"
#include "Hades/UniqueId.hpp"

struct snake_game_t {};

//type that holds all the game rules information
struct snake_rules : public hades::resources::resource_type<snake_game_t>
{
	hades::types::uint32 snake_length;
	hades::types::uint32 snake_speed_ms;
};

//global access to the game rules
snake_rules *snake_game_rules = nullptr;

//parser responsible for reading the rules from the mod file
void parseSnakeRules(hades::data::UniqueId, YAML::Node&, hades::data::data_manager*);

#endif //TEST_SNAKE_LOADER_HPP