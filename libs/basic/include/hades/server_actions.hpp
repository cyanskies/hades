#ifndef HADES_BASIC_SERVER_ACTIONS_HPP
#define HADES_BASIC_SERVER_ACTIONS_HPP

#include <vector>
#include <variant>

#include "hades/types.hpp"
#include "hades/uniqueid.hpp"

// a server action is a request sent to a server, they usually
// represent player input or commands to player owned entities

namespace hades
{
	namespace detail
	{
		using int_t = int32;
		using float_t = float32;
		using unique_t = unique_id;
		using vector_int_t = std::vector<int_t>;
		using vector_float_t = std::vector<float_t>;
		using vector_unique_t = std::vector<unique_t>;

		using variant_t = std::variant<int_t, float_t, unique_t,
			vector_int_t, vector_float_t, vector_unique_t>;
	}

	struct server_action
	{
		using value_type = detail::variant_t;

		//the action type eg. sword_attack, move, set_target
		unique_id id = unique_id::zero;

		std::vector<value_type> value;
	};
}

#endif //!HADES_BASIC_SERVER_ACTIONS_HPP