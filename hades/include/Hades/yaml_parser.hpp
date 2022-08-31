#ifndef HADES_YAML_PARSER_HPP
#define HADES_YAML_PARSER_HPP

#include <memory>
#include <string_view>

#include "hades/parser.hpp"

//TODO: remove
namespace YAML
{
	class Node;
}

namespace hades::data
{
	using yaml_parse_exception = parser_exception;

	class parser_node;
	//NOTE: throws yaml_parse_exception on error
	std::unique_ptr<parser_node> make_yaml_parser(std::string_view src);
	std::unique_ptr<parser_node> make_yaml_parser(std::istream& in);
	//TODO: remove
	[[deprecated]] std::unique_ptr<parser_node> make_yaml_parser(const YAML::Node &src);
}

#endif //HADES_YAML_PARSER_HPP
