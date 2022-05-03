#include "hades/debug/object_overlay.hpp"

#include <algorithm>

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/data.hpp"
#include "hades/for_each_tuple.hpp"
#include "hades/game_state.hpp"
#include "hades/level_interface.hpp"
#include "hades/utility.hpp"

using namespace std::string_literals;

namespace hades::debug
{
	template<typename T>
	string print_value(const T& value)
	{
		static_assert(resources::curve_types::is_curve_type_v<T>);
		if constexpr (resources::curve_types::is_collection_type_v<T>)
			return "[...]"s;// to_string(std::begin(t), std::end(t));
		else if constexpr (resources::curve_types::is_vector_type_v<T>)
			return "["s + to_string(value.x) + ", "s + to_string(value.y) + "]"s;
		else
			return to_string(value);
	}

	template<typename CurveType>
	string print_curve(std::string_view name, void* curve, time_point t)
	{
		auto variable = static_cast<CurveType*>(curve);

		if constexpr (is_pulse_curve_v<CurveType>)
		{
			auto pulses = variable->get(t);
			return to_string(name) + "["s + print_value(pulses.first.value) + ","s + print_value(pulses.second.value) + "]"s;
		}
		else
		{
			const auto& value = [](auto variable, time_point t)->typename CurveType::value_type {
				if constexpr (is_linear_curve_v<CurveType> || is_step_curve_v<CurveType>)
					return variable->get(t);
				else if constexpr (is_const_curve_v<CurveType>)
					return variable->get();
			}(variable, t);

			return to_string(name) + print_value(value);
		}
	}

	object_overlay::object_overlay(common_interface* l, game_interface* g, object_ref o) :
		_object{ o }, _level{ l }, _game{ g }
	{
		assert(l);

		//try and fix a bad object
		if (state_api::is_object_stale(_object) && _game)
			state_api::get_object_ptr(_object, g->get_extras());

		if (state_api::is_object_stale(_object))
			_object.ptr = nullptr;
	}

	string object_overlay::update()
	{
		if (_object.ptr == nullptr)
			return "object: unavailable"s;

		auto string = "___Object_Inspector___\n"s + _name + "\n"s;
		for (auto& c : _curves)
			string += c.print(c.name, c.curve, _time) + "\n"s;
		return string;
	}
}
