#include "hades/parser.hpp"

using namespace std::string_literals;

namespace hades::data
{
	// default parsers
	static make_parser_f make_parser_ptr = {};
	static make_parser2_f make_parser2_ptr = {};

	// per-ext parsers
	static std::map<std::filesystem::path, make_parser_f> make_parser_table;
	static std::map<std::filesystem::path, make_parser2_f> make_parser2_table;

	void set_default_parser(make_parser_f f)
	{
		make_parser_ptr = f;
	}

	void set_default_parser(make_parser2_f f)
	{
		make_parser2_ptr = f;
	}

	void set_parser(make_parser_f f, std::filesystem::path ext)
	{
		make_parser_table.emplace(std::move(ext), std::move(f));
		return;
	}

	void set_parser(make_parser2_f f, std::filesystem::path ext)
	{
		make_parser2_table.emplace(std::move(ext), std::move(f));
		return;
	}

	std::unique_ptr<parser_node> make_parser(std::string_view s, std::filesystem::path ext)
	{
		if (ext.empty())
		{
			// default parser
			assert(make_parser_ptr);
			return std::invoke(make_parser_ptr, s);
		}

		// parser for ext
		const auto iter = make_parser_table.find(ext);
		if (iter != end(make_parser_table))
			return std::invoke(iter->second, s);

		throw parser_exception{ "unable to find parser for ext: "s + ext.string() };
	}

	std::unique_ptr<parser_node> make_parser(std::istream& s, std::filesystem::path ext)
	{
		if (ext.empty())
		{
			// default parser
			assert(make_parser2_ptr);
			return std::invoke(make_parser2_ptr, s);
		}

		// parser for ext
		const auto iter = make_parser2_table.find(ext);
		if (iter != end(make_parser2_table))
			return std::invoke(iter->second, s);

		throw parser_exception{ "unable to find parser for ext: "s + ext.string() };
	}
}
