#include "hades/data_system.hpp"

#include <set>

#include "hades/data.hpp"
#include "hades/logging.hpp"
#include "hades/files.hpp"
#include "hades/utility.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace hades::data
{
	const auto no_id_string = "ERROR_NO_UNIQUE_ID"s;
	const auto unnamed_id_string = "ERROR_UNNAMED_UNIQUE_ID"s;

	data_system::data_system() : _ids({ {string{ no_id_string }, unique_id::zero} })
	{
		const auto& mod_stack = get_mod_stack();
		assert(!empty(mod_stack));
		const auto& built_in = mod_stack.front();
		const auto [iter, success] = _ids.emplace(std::make_pair(built_in->mod_info.name, built_in->mod_info.id));
		assert(success);
	}

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
			LOG("Failed to load " + to_string(game) + ", a game has already been loaded.");
			return;//game already loaded ignore
		}

		try
		{
			add_mod(game, true, "game.yaml");
			game_loaded = true;
		}
		catch (const files::file_error& f)
		{
			LOGERROR(f.what());
		}
		catch (const parser_exception& e)
		{
			const auto message = to_string(e.what()) + " while parsing: " + to_string(game) + "/game.yaml";
			LOGERROR(message);
		}
	}

	static bool ContainsTab(std::string_view yaml)
	{
		return std::find(yaml.begin(), yaml.end(), '\t') != yaml.end();
	}

	//mod is the name of a folder or archive containing a mod.yaml file
	void data_system::add_mod(std::string_view mod, bool autoLoad, std::string_view name)
	{
		// TODO: if a mod fails to load, we need to unload the mod and any dependencies that it brought it
		auto m = data::mod{};
		m.source = mod;
		const auto modyaml = files::read_resource(m, name);

		if (ContainsTab(modyaml))
			log_warning("Yaml file: "s + to_string(mod) + "/"s + to_string(name) + " contains tabs, expect errors."s);
		//parse game.yaml
		const auto root = data::make_parser(modyaml);
		const auto mod_key = get_uid(mod);

		_parse_mod(mod_key, mod, *root, autoLoad);
		
		//record the list of loaded games/mods
		if (name == "game.yaml")
		{
			assert(!_game);
			_game = mod_key;
		}
		else
			_mods.push_back(mod_key);
	}

	bool data_system::try_load_mod(std::string_view mod)
	{
		const auto old_stack = get_mod_count();
		try
		{
			add_mod(mod);
			return true;
		}
		catch (const runtime_error& e)
		{
			const auto new_stack = get_mod_count();
			const auto end = new_stack - old_stack;
			for (auto i = std::size_t{}; i < end; ++i)
				pop_mod();

			log_error("Unable to load mod: "s + to_string(mod));
			log_error(e.what());
			return false;
		}

	}

	bool data_system::loaded(std::string_view mod)
	{
		const auto iter = _ids.find(string{ mod });
		//name hasn't even been used yet
		if (iter == end(_ids))
			return false;

		try
		{
			get_mod(iter->second);
			return true;
		}
		//name has been used, but not for a mod
		catch (data::resource_error&)
		{
			return false;
		}
	}

	void data_system::reparse()
	{
		//copy and clear the mod list, so that
		// when add_mod readds them we don't double up
		auto mods = std::vector<unique_id>{};
		std::swap(mods, _mods);

		//reparse the game
		const auto& game = get_mod(_game).source;
		add_mod(game, true, "game.yaml");

		//go through the mod list and reload them in the
		//same order as they were origionally loaded
		//(this means we will parse the game and it's dependents again)
		for (const auto& m : mods)
		{
			//for each mod reload it
			const auto& name = get_mod(m).source;
			add_mod(name);
		}
	}

	void data_system::refresh()
	{
		//note: load queue can be full of duplicates,
		//the load functions resolve them
		for (auto& id : _ids)
			refresh(id.second);
	}

	void data_system::refresh(unique_id id)
	{
		if (exists(id))
			_loadQueue.push_back(get_resource(id));
	}

	void data_system::load()
	{
		auto res = std::vector<resources::resource_base*>{ std::move(_loadQueue) };
		_loadQueue = {};
		
		std::sort(begin(res), end(res));
		const auto last = std::unique(begin(res), end(res));
		std::for_each(begin(res), last, [this](auto resource) {
			resource->load(*this);
			return;
			});
		return;
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
	const string &data_system::get_as_string(const unique_id uid) const noexcept
	{
		for (const auto &id : _ids)
		{
			if (id.second == uid)
				return id.first;
		}

		if(uid == unique_zero)
			return no_id_string;

		return unnamed_id_string;
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

		auto str = to_string(name);
		auto iter = _ids.find(str);
		if (iter != end(_ids))
			return iter->second;

		const auto out = _ids.emplace(std::move(str), make_unique_id());
		assert(out.second);
		return out.first->second;
	}

	template<typename YAMLPARSER>
	static void parseInclude(unique_id mod, types::string file, const data::mod& mod_info, YAMLPARSER &&yamlParser)
	{
		try
		{
			const auto include_yaml = files::read_resource(mod_info, file);

			if (ContainsTab(include_yaml))
				LOGWARNING("Yaml file: " + mod_info.name + "/" + file + " contains tabs, expect errors.");

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
			const auto message = to_string(e.what()) + " while parsing: " + mod_info.name + "/" + file;
			LOGERROR(message);
		}

		return;
	}

	void data_system::_parse_mod(unique_id modKey, std::string_view source, const data::parser_node& modRoot, bool load_deps)
	{
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

		auto mod_info = data::mod{};
		mod_info.source = source;
		mod_info.id = modKey;
		
		//parse the mod header;
		const auto default_mod_name = "mod_name_missing"s;
		mod_info.name = parse_tools::get_scalar<string>(*mod, "name", default_mod_name);

		assert(mod_info.name != default_mod_name);

		//check mod dependencies
		const auto dependencies = mod->get_child("depends");
		if (dependencies)
		{
			const auto deps = dependencies->to_sequence<string>();
			for (const auto& s : deps)
			{
				if (load_deps)
					add_mod(s, load_deps);
				else if(!loaded(s)) {
					log_error("One of mods: "s + to_string(source) + ", dependencies has not been provided, was: "s + s);
				}
			}
		}
		
		// copy name for later
		const auto mod_name = mod_info.name;
		push_mod(std::move(mod_info));

		//for every other headers, check for a header parser
		parseYaml(modKey, modRoot);
		
		log("Loaded mod: "s + mod_name);
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
				const auto yaml_parser = [this](unique_id mod, const data::parser_node& root) { parseYaml(mod, root); };

				auto& mod_res = get_mod(mod);
				const auto includes = header->to_sequence<string>();
				for (const auto& s : includes)
				{
					// record the include files used by this mod
					mod_res.includes.emplace_back(s);
					parseInclude(mod, s, mod_res, yaml_parser);
				}
			}

			//if this resource name has a parser then load it
			auto &parser = _resourceParsers[type];
			if (parser)
				std::invoke(parser, mod, *header, *this);
		}

		return;
	}
}
