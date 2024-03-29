#include "hades/mission.hpp"

#include <string_view>

#include "hades/data.hpp"
#include "hades/level.hpp"
#include "hades/parser.hpp"
#include "hades/writer.hpp"

namespace hades
{
	using namespace std::string_view_literals;

	constexpr auto name_str = "name"sv,
		desc_str = "description"sv,
		players_str = "players"sv,
		obj_str = "objects"sv,
		next_str = "next-id"sv,
		levels_str = "inline-levels"sv,
		ext_levels_str = "external-levels"sv;

	constexpr auto player_name_str = "name"sv,
		player_object_str = "object"sv;

	static string serialise_impl(const mission& m, data::writer& w)
	{
		//mission.mis
		//name: string
		//description: string
		//players:
		//	id:
		//		name:
		//		object:
		//objects: //TODO:
		//next-id: 
		//scripts:
		//inline-levels:
		//external-levels:

		if(!empty(m.name))
			w.write(name_str, m.name);
		if(!empty(m.description))
			w.write(desc_str, m.description);

		//players
		if (!std::empty(m.players))
		{
			w.start_map(players_str);
			for (const auto& p : m.players)
			{
				w.start_map(p.id);
				w.write(player_object_str, p.object);
				w.end_map();
			}
			w.end_map();
		}

		serialise(m.objects, w);

		if (!std::empty(m.inline_levels))
		{
			w.start_map(levels_str);
			for (const auto& l : m.inline_levels)
			{
				w.start_map(l.name);
                serialise(l.level_obj, w);
				w.end_map();
			}
			w.end_map();
		}

		if (!std::empty(m.external_levels))
		{
			w.start_map(ext_levels_str);
			for (const auto& l : m.external_levels)
				w.write(l.name, l.path);
			w.end_map();
		}

		return w.get_string();
	}

	string serialise(const mission& m)
	{
		auto w = data::make_writer();
		assert(w);
		serialise_impl(m, *w);
		return w->get_string();
	}

	void serialise(const mission& m, data::writer& w)
	{
		serialise_impl(m, w);
		return;
	}

	static mission deserialise_mission_impl(const std::unique_ptr<hades::data::parser_node> parser)
	{
		auto m = mission{};

		namespace pt = data::parse_tools;

		m.name = pt::get_scalar(*parser, name_str, m.name);
		m.description = pt::get_scalar(*parser, desc_str, m.description);

		//players
		const auto players = parser->get_child(players_str);
		if (players)
		{
			const auto ps = players->get_children();
			for (const auto& p : ps)
			{
				mission::player player;
				player.id = p->to_scalar<unique_id>(data::make_uid);
				player.object = pt::get_scalar(*p, player_object_str, player.object);
				m.players.emplace_back(std::move(player));
			}
		}

		m.objects = deserialise_object_data(*parser);

		const auto inline_levels = parser->get_child(levels_str);
		if (inline_levels)
		{
			const auto ls = inline_levels->get_children();
			for (const auto& l : ls)
			{
				const auto name = l->to_string();
				auto level = deserialise_level(*l);
				m.inline_levels.emplace_back(mission::level_element{ 
					data::make_uid(name), 
					std::move(level) 
					});
			}
		}

		return m;
	}

	mission deserialise_mission(std::string_view s)
	{
		return deserialise_mission_impl(data::make_parser(s));
	}

	mission deserialise_mission(std::unique_ptr<data::parser_node> n)
	{
		return deserialise_mission_impl(std::move(n));
	}

	mission_save make_save_from_mission(mission l)
	{
		auto m = mission_save{};

		//convert objects to save format
		m.objects.next_id = l.objects.next_id;
		m.objects.objects.reserve(std::size(l.objects.objects));
		for (auto& obj : l.objects.objects)
			m.objects.objects.push_back(make_save_instance(std::move(obj)));

		m.source = std::move(l);
		//the server should convert leveld from source into saves as needed
		//m.level_saves
		return m;
	}
}
