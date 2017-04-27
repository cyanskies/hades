#ifndef TEST_INPUT_HPP
#define TEST_INPUT_HPP

#include "Hades/UniqueId.hpp"


struct Input
{
	using ActionId = hades::data::UniqueId;

	static ActionId move_left;
	static ActionId move_up;
	static ActionId move_right;
	static ActionId move_down;
};

#endif //TEST_INPUT_HPP