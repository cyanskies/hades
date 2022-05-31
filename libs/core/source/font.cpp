#include "hades/font.hpp"

#include "SFML/System/Err.hpp"

#include "hades/files.hpp"
#include "hades/parser.hpp"
#include "hades/utility.hpp"

static hades::unique_id default_font_id = hades::unique_zero;

namespace hades
{
	void parse_font(unique_id, const data::parser_node&, data::data_manager&);

	void register_font_resource(data::data_manager &d)
	{
		using namespace std::string_view_literals;
		default_font_id = d.get_uid("default-font");
		d.register_resource_type("fonts"sv, parse_font);
	}

	void load_font(resources::resource_type<sf::Font> &f, data::data_manager &d)
	{
		assert(dynamic_cast<resources::font*>(&f));
		auto &font = static_cast<resources::font&>(f);
		const auto mod = d.get<resources::mod>(font.mod);

		try
		{
			//we store the font memory for the gui class to use
			font.source_buffer = files::raw_resource(mod, font.source);
			auto sb = std::stringbuf{};
			const auto prev = sf::err().rdbuf(&sb);
			const auto finally = make_finally([prev]() noexcept {
				sf::err().set_rdbuf(prev);
				return;
				});

			if (!font.value.loadFromMemory(font.source_buffer.data(), font.source_buffer.size()))
				throw data::resource_error{ sb.str() };
		}
		catch (const files::file_error &e)
		{
			LOGERROR("Failed to load font: " + mod->source + "/" + font.source + ". " + e.what());
		}
	}

	void parse_font(unique_id m, const data::parser_node &n, data::data_manager &d)
	{
		//fonts yaml
		//fonts:
		//    name: source
		//    name2: source2

		const auto fonts = n.get_children();
		for (const auto &f : fonts)
		{
			const auto name = f->to_string();
			const auto value = f->get_child();
			if (!value)
				continue;

			const auto source = value->to_string();

			const auto id = d.get_uid(name);
			auto font = d.find_or_create<resources::font>(id, m);
			if (!font)
				continue;

			font->source = source;
		}
	}
}

namespace hades::resources
{
	unique_id default_font_id()
	{
		return ::default_font_id;
	}

	font::font() : resource_type<sf::Font>(load_font) {}
}