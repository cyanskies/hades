#include "hades/data_system.hpp"

#include <set>

#include "hades/data.hpp"
#include "hades/logging.hpp"
#include "hades/files.hpp"
#include "hades/utility.hpp"
#include "hades/yaml_parser.hpp"

namespace hades::data
{
	data_system::data_system() : _ids({ {no_id_string, unique_id::zero} })
	{}

	//application registers the custom resource types
	//parser must convert yaml into a resource manifest object
	void data_system::register_resource_type(std::string_view name, resources::parser_func parser)
	{
		_resourceParsers[to_string(name)] = parser;
	}

	//game is the name of a folder or archive containing a game.yaml file
	void data_system::load_game(std::string_view game)
	{
		static bool game_loaded = false;

		if (game_loaded)
		{
			LOG("Tried to load" + to_string(game) + ", Already loaded a game, skipping");
			return;//game already loaded ignore
		}

		try
		{
			add_mod(game, true, "game.yaml");
			game_loaded = true;
		}
		catch (const files::file_error & f)
		{
			LOGERROR(f.what());
		}
		catch (const parser_exception & e)
		{
			const auto message = to_string(e.what()) + " while parsing: " + to_string(game) + "/game.yaml";
			LOGERROR(message);
		}
	}

	static bool ContainsTab(std::string_view yaml)
	{
		return std::find(yaml.begin(), yaml.end(), '\t') != yaml.end();
	}

	template<typename GetId, typename FindMod, typename DepCheckFunc, typename ParseYamlFunc>
	static void parse_mod(std::string_view source, const data::parser_node& modRoot,
		GetId&& get_id, FindMod&& find_mod, DepCheckFunc&& dependencycheck, ParseYamlFunc&& parse_yaml)
	{
		auto modKey = std::invoke(std::forward<GetId>(get_id), source);

		//mod:
		//    name:
		//    depends:
		//        -other_mod

		//read the mod header
		const auto mod = modRoot.get_child("mod");
		if (!mod)
		{
			//missing mod header
			LOGERROR("mod header missing for mod: " + to_string(source));
			return;
		}

		auto mod_data = std::invoke(std::forward<FindMod>(find_mod), modKey);
		//parse the mod header;
		mod_data->source = source;
		mod_data->id = modKey;
		mod_data->name = parse_tools::get_scalar<string>(*mod, "name", mod_data->name);

		//check mod dependencies
		const auto dependencies = mod->get_child("depends");
		if (dependencies)
		{
			auto deps = dependencies->to_sequence<string>();
			for (auto& s : deps)
			{
				if (!std::invoke(std::forward<DepCheckFunc>(dependencycheck), std::move(s))) {
					LOGERROR("One of mods: " + to_string(source) + ", dependencies has not been provided, was: " + s);
				}
			}
		}

		//for every other headers, check for a header parser
		std::invoke(std::forward<ParseYamlFunc>(parse_yaml), modKey, modRoot);

		LOG("Loaded mod: " + mod_data->name);
		return;
	}

	//mod is the name of a folder or archive containing a mod.yaml file
	void data_system::add_mod(std::string_view mod, bool autoLoad, std::string_view name)
	{
		auto m = resources::mod{};
		m.source = mod;
		const auto modyaml = files::read_resource(&m, name);

		if (ContainsTab(modyaml))
			LOGWARNING("Yaml file: " + to_string(mod) + "/" + to_string(name) + " contains tabs, expect errors.");
		//parse game.yaml
		const auto root = data::make_parser(modyaml);

		const auto get_id = [this](std::string_view s) { return get_uid(s); };
		const auto find_mod = [this](unique_id m) { return find_or_create<resources::mod>(m, unique_zero); };
		const auto parse_yaml = [this](unique_id m, const data::parser_node& n) { return parseYaml(m, n); };

		if (autoLoad)
			//if auto load, then use the mod loader as the dependency check(will parse the dependent mod)
			parse_mod(mod, *root, get_id, find_mod, [this](std::string_view s) {this->add_mod(s, true); return true; }, parse_yaml);
		else
			//otherwise bind to loaded, returns false if dependent mod is not loaded
			parse_mod(mod, *root, get_id, find_mod, [this](std::string_view s) { return loaded(s); }, parse_yaml);

		//record the list of loaded games/mods
		if (name == "game.yaml")
			_game = get_uid(mod);
		else
			_mods.push_back(get_uid(mod));
	}

	bool data_system::loaded(std::string_view mod)
	{
		const auto iter = _ids.find(to_string(mod));
		//name hasn't even been used yet
		if (iter == end(_ids))
			return false;

		try
		{
			const auto m = get<resources::mod>(iter->second, no_load);
			return m->loaded;
		}
		//name has been used, but not for a mod
		catch (data::resource_wrong_type&)
		{
			return false;
		}
	}

	void data_system::reparse()
	{
		//copy and clear the mod list, so that
		// when add_mod readds them we don't double up
		auto mods = _mods;

		//reparse the game
		auto game = get<resources::mod>(_game)->name;
		add_mod(game, true, "game.yaml");

		_mods.clear();
		//go through the mod list and reload them in the
		//same order as they were origionally loaded
		//(this means we will parse the game and it's dependents again)
		for (auto m : mods)
		{
			//for each mod reload it
			auto name = get<resources::mod>(m)->name;
			add_mod(name);
		}
	}

	void data_system::refresh()
	{
		//note: load queue can be full of duplicates,
		//the load functions resolve them
		for (auto id : _ids)
			refresh(id.second);
	}

	void data_system::refresh(unique_id id)
	{
		if (exists(id))
		{
			//_resources stores unique_ptrs to resources
			//so we void ptr cast to get it out of the type erased property bag
			auto r = get_resource(id);

			_loadQueue.push_back(r);
		}
	}

	void data_system::load()
	{
		const std::set<resources::resource_base*> queue{ _loadQueue.begin(), _loadQueue.end() };
		_loadQueue.clear();

		for (auto r : queue)
			r->load(*this);
	}

	void data_system::load(unique_id id)
	{
		auto it = std::find_if(_loadQueue.begin(), _loadQueue.end(), [id](const resources::resource_base* r) {return r->id == id; });
		auto resource = *it;
		if (it == _loadQueue.end())
			return;

		//erase all the matching id's
		_loadQueue.erase(std::remove(_loadQueue.begin(), _loadQueue.end(), *it), _loadQueue.end());

		resource->load(*this);
	}

	void data_system::load(types::uint8 count)
	{
		remove_duplicates(_loadQueue);

		while (count-- > 0 && !_loadQueue.empty())
		{
			_loadQueue.back()->load(*this);
			_loadQueue.pop_back();
		}
	}

	//convert string to uid
	types::string data_system::get_as_string(unique_id uid) const
	{
		for (auto id : _ids)
		{
			if (id.second == uid)
				return id.first;
		}

		return no_id_string;
	}

	unique_id data_system::get_uid(std::string_view name) const
	{
		if (name.empty())
			return unique_id::zero;

		auto id = _ids.find(types::string(name));

		if (id == std::end(_ids))
			return unique_id::zero;

		return id->second;
	}

	unique_id data_system::get_uid(std::string_view name)
	{
		if (name.empty())
			return unique_id::zero;

		return _ids[types::string(name)];
	}

	template<typename GETMOD, typename YAMLPARSER>
	static void parseInclude(unique_id mod, types::string file, GETMOD &&getMod, YAMLPARSER &&yamlParser)
	{
		const auto mod_info = std::invoke(std::forward<GETMOD>(getMod), mod);
		try
		{
			const auto include_yaml = files::read_resource(mod_info, file);

			if (ContainsTab(include_yaml))
				LOGWARNING("Yaml file: " + mod_info->name + "/" + file + " contains tabs, expect errors.");

			const auto parser = data::make_parser(include_yaml);
			std::invoke(std::forward<YAMLPARSER>(yamlParser), mod, *parser);
		}
		catch (const files::file_error& f)
		{
			LOGERROR(f.what());
			return;
		}
		catch (const parser_exception& e)
		{
			const auto message = to_string(e.what()) + " while parsing: " + mod_info->name + "/" + file;
			LOGERROR(message);
		}

		return;
	}

	void data_system::parseYaml(unique_id mod, const data::parser_node& root)
	{
		//loop though each of the root level nodes and pass them off
		//to specialised handlers if present
		for (const auto& header : root.get_children())
		{
			const auto type = header->to_string();

			//don't try to read mod headers
			if (type == "mod")
				continue;

			//if type is include, then open the file and pass it to parseyaml
			if (type == "include")
			{
				const auto get_mod = [this](unique_id mod) {return get<resources::mod>(mod); };
				const auto yaml_parser = [this](unique_id mod, const data::parser_node& root) { parseYaml(mod, root); };

				const auto includes = header->to_sequence<string>();
				for (const auto& s : includes)
					parseInclude(mod, s, get_mod, yaml_parser);
			}

			//if this resource name has a parser then load it
			auto &parser = _resourceParsers[type];
			if (parser)
				std::invoke(parser, mod, *header, *this);
		}

		return;
	}
}
