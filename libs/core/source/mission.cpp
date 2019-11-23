#include "hades/mission.hpp"

#include <string_view>

#include "hades/parser.hpp"
#include "hades/writer.hpp"

namespace hades
{
	using namespace std::string_view_literals;

	constexpr auto name_str = "name"sv,
		desc_str = "description"sv,
		obj_str = "objects"sv,
		next_str = "next-id"sv,
		levels_str = "inline-levels"sv,
		ext_levels_str = "external-levels"sv;

	string serialise(const mission& m)
	{
		//mission.mis
		//name: string
		//description: string
		//objects: //TODO:
		//next-id: 
		//scripts:
		//inline-levels:
		//external-levels:

		auto w = data::make_writer();
		w->write(name_str, m.name);
		w->write(desc_str, m.description);

		//TODO: next-id and objects

		if (!std::empty(m.inline_levels))
		{
			w->start_map(levels_str);
			for (const auto& l : m.inline_levels)
			{
				w->start_map(l.name);
				serialise(l.level, *w);
				w->end_map();
			}
			w->end_map();
		}

		if (!std::empty(m.external_levels))
		{
			w->start_map(ext_levels_str);
			for (const auto& l : m.external_levels)
				w->write(l.name, l.path);
			w->end_map();
		}

		return w->get_string();
	}

	mission deserialise_mission(std::string_view s)
	{
		const auto p = data::make_parser(s);
		auto m = mission{};

		namespace pt = data::parse_tools;

		m.name = pt::get_scalar(*p, name_str, m.name);
		m.description = pt::get_scalar(*p, desc_str, m.description);

		const auto inline_levels = p->get_child(levels_str);
		if (inline_levels)
		{
			const auto ls = inline_levels->get_children();
			for (const auto& l : ls)
			{
				const auto name = l->to_string();
				auto level = deserialise_level(*l);
			}
		}

		return m;
	}

	mission_save make_save_from_mission(mission l)
	{
		return mission_save{};
	}
}