#include "hades/parser.hpp"

namespace hades::data::parse_tools
{
	unique_id get_unique(const parser_node &node,
		std::string_view property_name, unique_id default_value)
	{
		return get_scalar(node, property_name, default_value, [](std::string_view str) {
			return make_uid(str);
		});
	}

	std::vector<unique_id> get_unique_sequence(const parser_node &node, std::string_view property_name,
		const std::vector<unique_id> default_value)
	{
		//TODO: support the sequence operators
		// + - =
		//remove dupelicates
		return get_sequence(node, property_name, default_value, [](std::string_view s) {
			return make_uid(s);
		});
	}
}