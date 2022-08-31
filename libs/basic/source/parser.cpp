#include "hades/parser.hpp"

namespace hades::data
{
	static make_parser_f make_parser_ptr = {};
	static make_parser2_f make_parser2_ptr = {};

	void set_default_parser(make_parser_f f)
	{
		make_parser_ptr = f;
	}

	void set_default_parser(make_parser2_f f)
	{
		make_parser2_ptr = f;
	}

	std::unique_ptr<parser_node> make_parser(std::string_view s)
	{
		assert(make_parser_ptr);
		return std::invoke(make_parser_ptr, s);
	}

	std::unique_ptr<parser_node> make_parser(std::istream& s)
	{
		assert(make_parser2_ptr);
		return std::invoke(make_parser2_ptr, s);
	}
}
