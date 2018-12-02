#ifndef HADES_YAML_PARSER_HPP
#define HADES_YAML_PARSER_HPP

#include <memory>
#include <string_view>

//TODO: remove
namespace YAML
{
	class Node;
}

namespace hades::data
{
	//thrown by make_yaml_parser
	//TODO: replace with generic parser_exception in parser.hpp
	//for compat with make_default_parser
	class yaml_parse_exception : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	class parser_node;
	std::unique_ptr<parser_node> make_yaml_parser(std::string_view src);
	//TODO: remove
	std::unique_ptr<parser_node> make_yaml_parser(const YAML::Node &src);
}

#endif //HADES_YAML_PARSER_HPP
