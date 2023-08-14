#ifndef HADES_YAML_PARSER_HPP
#define HADES_YAML_PARSER_HPP

#include <memory>
#include <string_view>

#include "hades/parser.hpp"

namespace hades::data
{
	using yaml_parse_exception = parser_exception;

	//NOTE: throws yaml_parse_exception on error
	std::unique_ptr<parser_node> make_yaml_parser(std::string_view src);
	std::unique_ptr<parser_node> make_yaml_parser(std::istream& in);
}

#endif //HADES_YAML_PARSER_HPP
