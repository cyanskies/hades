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

		if (!state_api::is_object_stale(_object))
		{
			//load up all our data

			// count the total number of curves
			const auto elms = for_each_tuple_r(_object.ptr->object_variables, [](const auto& container) {
				return size(container);
				});
			const auto count = std::reduce(begin(elms), end(elms));
			auto curves = std::vector<curve_struct>{};
			curves.reserve(count);

			//generate print functions for each variable
			for_each_tuple(_object.ptr->object_variables, [&curves](const auto& container) {
				for (auto& elm : container)
				{
					assert(elm.var);
					curves.push_back(curve_struct{
							data::get_as_string(elm.id) + ": "s,
							&elm.var->data,
							print_curve<typename std::decay_t<decltype(elm)>::value_type>
						});
				}
			});

			//alphabetical sort
			std::sort(begin(curves), end(curves), [](const auto& l, const auto& r) {
				return l.name < r.name;
			});

			//store the sorted curves and print functions
			_curves = std::move(curves);

			//find the objects name if it has one
			/*const auto iter = std::find_if(begin(_level->get_state().names),
				end(_level->get_state().names), [id = _object.id](auto& pair){
				return id == pair.second.get().id; // needs time
			});*/

			if (true)//iter == end(_level->get_state().names))
				_name = "object: "s + to_string(_object.id);
			/*else
				_name = "object: "s + iter->first;*/
		}
		else
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
