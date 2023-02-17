#include "hades/font.hpp"

#include "SFML/System/Err.hpp"

#include "hades/files.hpp"
#include "hades/parser.hpp"
#include "hades/utility.hpp"
#include "hades/writer.hpp"

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

	void load_font(resources::font& font, data::data_manager &d)
	{
		const auto& mod = d.get_mod(font.mod);

		try
		{
			//we store the font memory for the gui class to use
			font.source_buffer = files::raw_resource(mod, font.source);
			auto sb = std::stringbuf{};
			const auto prev = sf::err().rdbuf(&sb);
			const auto finally = make_finally([prev]() noexcept {
                sf::err().rdbuf(prev);
				return;
				});

			if (!font.value.loadFromMemory(font.source_buffer.data(), font.source_buffer.size()))
				throw data::resource_error{ sb.str() };
		}
		catch (const files::file_error &e)
		{
			log_error("Failed to load font: " + mod.source + "/" + font.source.generic_string() + ". " + e.what());
		}
	}

	using namespace std::string_view_literals;

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
			auto font = d.find_or_create<resources::font>(id, m, "fonts"sv);
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

	void font::load(data::data_manager& d)
	{
		load_font(*this, d);
		return;
	}

	void font::serialise(const data::data_manager& d, data::writer& w) const
	{
		w.start_map(d.get_as_string(id));
		w.write("source"sv, source.generic_string());
		w.end_map();
		return;
	}

	void font::serialise(std::ostream& o) const
	{
		const auto fstrm = files::stream_resource(data::get_mod(mod), source);
		o << fstrm.rdbuf();
		return;
	}
}
