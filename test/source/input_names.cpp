#include "input_names.hpp"

//assign the action names to uniqueid zero
//you'll have to retrieve their names from the data manager when regestering the resources
auto zero = Input::ActionId::zero;

Input::ActionId Input::move_left = zero;
Input::ActionId Input::move_up = zero;
Input::ActionId Input::move_right = zero;
Input::ActionId Input::move_down = zero;