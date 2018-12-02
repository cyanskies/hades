#include "hades/parser.hpp"


namespace hades::data
{
	static make_parser_f make_parser_ptr = nullptr;

	void set_default_parser(make_parser_f f)
	{
		make_parser_ptr = f;
	}

	std::unique_ptr<parser_node> make_parser(std::string_view s)
	{
		assert(make_parser_ptr);
		return std::invoke(make_parser_ptr, s);
	}
}

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