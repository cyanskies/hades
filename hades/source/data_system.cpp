#include "Hades/data_system.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/Data.hpp"
#include "Hades/Logging.hpp"
#include "Hades/files.hpp"

namespace hades
{
	namespace data
	{
		data_system::data_system() : _ids({ {no_id_string, UniqueId::Zero} })
		{}

		//application registers the custom resource types
		//parser must convert yaml into a resource manifest object
		void data_system::register_resource_type(std::string_view name, resources::parserFunc parser)
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
			catch (files::file_exception &f)
			{
				LOGERROR(f.what());
			}
			catch (YAML::Exception &e)
			{
				const auto message = to_string(e.what()) + " while parsing: " + to_string(game) + "/game.yaml";
				LOGERROR(message);
			}
		}

		inline bool ContainsTab(std::string_view yaml)
		{
			return std::find(yaml.begin(), yaml.end(), '\t') != yaml.end();
		}

		//mod is the name of a folder or archive containing a mod.yaml file
		void data_system::add_mod(std::string_view mod, bool autoLoad, std::string_view name)
		{
			const auto modyaml = files::as_string(mod, name);

			if (ContainsTab(modyaml))
				LOGWARNING("Yaml file: " + to_string(mod) + "/" + to_string(name) + " contains tabs, expect errors.");
			//parse game.yaml
			auto root = YAML::Load(modyaml.c_str());
			if (autoLoad)
				//if auto load, then use the mod loader as the dependency check(will parse the dependent mod)
				parseMod(to_string(mod), root, [this](std::string_view s) {this->add_mod(s, true); return true; });
			else
				//otherwise bind to loaded, returns false if dependent mod is not loaded
				parseMod(to_string(mod), root, std::bind(&data_system::loaded, this, std::placeholders::_1));

			//record the list of loaded games/mods
			if (name == "game.yaml")
				_game = getUid(mod);
			else
				_mods.push_back(getUid(mod));
		}

		bool data_system::loaded(std::string_view mod)
		{
			//name hasn't even been used yet
			if (_names.find(types::string{ mod }) == _names.end())
				return false;

			try
			{
				auto r = get<resources::mod>(getUid(mod));
			}
			//name has been used, but not for a mod
			catch (data::resource_wrong_type&)
			{
				return false;
			}

			//name is used and is a mod
			return true;
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

		void data_system::refresh(UniqueId id)
		{
			if (exists(id))
			{
				//_resources stores unique_ptrs to resources
				//so we void ptr cast to get it out of the type erased property bag
				auto r = getResource(id);

				_loadQueue.push_back(r);
			}
		}

		void data_system::load()
		{
			const std::set<resources::resource_base*> queue{ _loadQueue.begin(), _loadQueue.end() };
			_loadQueue.clear();

			for (auto r : queue)
				r->load(this);
		}

		void data_system::load(data::UniqueId id)
		{
			auto it = std::find_if(_loadQueue.begin(), _loadQueue.end(), [id](const resources::resource_base* r) {return r->id == id; });
			auto resource = *it;
			if (it == _loadQueue.end())
				return;

			//erase all the matching id's
			_loadQueue.erase(std::remove(_loadQueue.begin(), _loadQueue.end(), *it), _loadQueue.end());

			resource->load(this);
		}

		void data_system::load(types::uint8 count)
		{
			std::sort(_loadQueue.begin(), _loadQueue.end());
			std::unique(_loadQueue.begin(), _loadQueue.end());

			while (count-- > 0 && !_loadQueue.empty())
			{
				_loadQueue.back()->load(this);
				_loadQueue.pop_back();
			}
		}

		//convert string to uid
		types::string data_system::getAsString(UniqueId uid) const
		{
			for (auto id : _ids)
			{
				if (id.second == uid)
					return id.first;
			}

			return no_id_string;
		}

		UniqueId data_system::getUid(std::string_view name) const
		{
			auto id = _ids.find(types::string(name));

			if (id == std::end(_ids))
				return UniqueId::Zero;

			return id->second;
		}

		UniqueId data_system::getUid(std::string_view name)
		{
			return _ids[types::string(name)];
		}

		template<typename GETMOD, typename YAMLPARSER>
		void parseInclude(data::UniqueId mod, types::string file, GETMOD getMod, YAMLPARSER yamlParser)
		{
			std::string include_yaml;
			auto mod_info = getMod(mod);
			try
			{
				include_yaml = files::as_string(mod_info->source, file);
			}
			catch (files::file_exception &f)
			{
				LOGERROR(f.what());
				return;
			}

			if (ContainsTab(include_yaml))
				LOGWARNING("Yaml file: " + mod_info->name + "/" + file + " contains tabs, expect errors.");

			yamlParser(mod, YAML::Load(include_yaml.c_str()));
		}

		void data_system::parseYaml(data::UniqueId mod, YAML::Node root)
		{
			//loop though each of the root level nodes and pass them off
			//to specialised handlers if present
			for (auto header : root)
			{
				//don't try to read mod headers
				if (header.first.as<types::string>() == "mod")
					continue;

				auto type = header.first.as<types::string>();
				//if type is include, then open the file and pass it to parseyaml
				if (type == "include")
				{
					if (header.second.IsScalar())
					{
						auto value = header.second.as<types::string>();
						parseInclude(mod, value, [this](data::UniqueId mod) {return get<resources::mod>(mod); }, [this](data::UniqueId mod, YAML::Node root) {parseYaml(mod, root); });
					}
					else if (header.second.IsSequence())
					{
						;
					}
				}

				//if this resource name has a parser then load it
				auto parser = _resourceParsers[type];
				if (parser)
					parser(mod, header.second, this);
			}
		}

		void data_system::parseMod(std::string_view source, YAML::Node modRoot, std::function<bool(std::string_view)> dependencycheck)
		{
			auto modKey = getUid(source);

			//mod:
			//    name:
			//    depends:
			//        -other_mod

			//read the mod header
			auto mod = modRoot["mod"];
			if (mod.IsNull() || !mod.IsDefined())
			{
				//missing mod header
				LOGERROR("mod header missing for mod: " + to_string(source));
				return;
			}

			//parse the mod header;
			auto mod_data = std::make_unique<resources::mod>();
			mod_data->source = source;
			mod_data->id = modKey;
			auto mod_name = mod["name"];
			if (mod_name.IsNull() || !mod_name.IsDefined())
			{
				//missing mod name
				LOGERROR("name missing for mod: " + to_string(source));
				return;
			}

			auto name = mod_name.as<types::string>();
			mod_data->name = name;

			//check mod dependencies
			auto dependencies = mod["depends"];
			if (dependencies.IsDefined())
			{
				if (dependencies.IsSequence())
				{
					for (const auto &d : dependencies)
					{
						if (!dependencycheck(d.as<types::string>()))
						{
							LOGERROR("One of mods: " + to_string(source) + " dependencies has not been provided, was: " + d.as<types::string>());
							continue;
						}
					}
				}
				else
					LOGERROR("Tried to read dependencies for mod: " + to_string(source) + ", but the depends entry did not contain a sequence");
			}

			//store the data
			set<resources::mod>(modKey, std::move(mod_data));

			//for every other headers, check for a header parser
			parseYaml(modKey, modRoot);

			LOG("Loaded mod: " + name);
		}
	}//namespace !data
}

using namespace hades;

//posts error message if the yaml node is the wrong type
bool yaml_error(types::string resource_type, types::string resource_name,
	types::string property_name, types::string requested_type, data::UniqueId mod, bool test)
{
	if (!test)
	{
		using namespace std::string_literals;
		auto message = "Error passing YAML"s;
		if (mod != hades::UniqueId::Zero)
		{
			auto mod_ptr = data::Get<resources::mod>(mod);
			message += ", in mod: "s + mod_ptr->name;
		}
		message += ", type: "s + resource_type + ", name: "s
			+ resource_name + ", for property: "s + property_name + ". value must be "s + requested_type;
		LOGERROR(message);
	}

	return test;
}

hades::data::UniqueId yaml_get_uid(const YAML::Node& node, hades::types::string resource_type, hades::types::string resource_name,
	hades::types::string property_name, hades::data::UniqueId mod, hades::data::UniqueId default_value)
{
	auto value_node = node[property_name];
	if (value_node.IsDefined() && yaml_error(resource_type, resource_name, property_name, "scalar", mod, value_node.IsScalar()))
	{
		auto str = value_node.as<hades::types::string>();
		if (!str.empty())
			return hades::data::MakeUid(str);
	}

	return default_value;
}
