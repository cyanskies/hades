#include "hades/debug/object_overlay.hpp"

#include <algorithm>

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/data.hpp"
#include "hades/game_state.hpp"
#include "hades/level_interface.hpp"
#include "hades/time.hpp"
#include "hades/utility.hpp"

using namespace std::string_literals;

namespace hades::debug
{
	object_overlay::object_overlay(common_interface* l, game_interface* g, object_ref o, bool sticky) :
		_object{ o }, _level{ l }, _game{ g }, _sticky{ sticky }
	{
		assert(l);

		//try and fix a bad object
		if (state_api::is_object_stale(_object) && _game)
			state_api::get_object_ptr(_object, g->get_extras());
		
		if(_object.ptr)
			_type = to_string(hades::resources::object_functions::get_id(*_object.ptr->object_type));		
	}

	class print_curve_visitor
	{
	public:
		void* var;
		string& str;
		time_point t;

		template<template<typename> typename CurveType, typename T>
			requires std::same_as<CurveType<float>, linear_curve<float>> 
					|| std::same_as<CurveType<float>, step_curve<float>>
		void print_curve(const CurveType<T>&c)
		{
			if constexpr (resources::curve_types::is_collection_type_v<T>)
				str += "[...]"s;
			else if constexpr (resources::curve_types::is_vector_type_v<T>)
				str += vector_to_string(c.get(t));
			else
				str += to_string(c.get(t));

			str += "\n"s;
			return;
		}

		template<typename T>
		void print_curve(const pulse_curve<T>& c)
		{
			const auto pulses = c.get(t);

			if constexpr (resources::curve_types::is_collection_type_v<T>)
				str += "[...]"s;
			else if constexpr (resources::curve_types::is_vector_type_v<T>)
				str += vector_to_string(pulses.second.value);
			else
				str += to_string(pulses.second.value);

			str += "\n"s;

			return;
		}
		
		template<template <typename> typename CurveType, typename VarType>
			void operator()()
		{
			auto state_var = static_cast<state_field<CurveType<VarType>>*>(var);
			str += to_string(state_var->id) + " = "s;
			print_curve(state_var->data);
			return;
		}
	};

	string object_overlay::update()
	{
		if (state_api::is_object_stale(_object))
			return "object("s + hades::to_string(hades::to_value(_object.id)) + "): unavailable"s;

		auto string = "___Object_Inspector___\n"s + _type +
			": (id: "s + hades::to_string(hades::to_value(_object.id)) + ") "s + _name + "\n"s;

		for (auto& entry : _object.ptr->object_variables)
			state_api::detail::call_with_curve_info(entry.info, print_curve_visitor{ entry.var, string, _time });

		return string;
	}
}
