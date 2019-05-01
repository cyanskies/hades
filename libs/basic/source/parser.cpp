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
