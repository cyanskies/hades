#include "Tiles/serialise.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/Data.hpp"
#include "Hades/DataManager.hpp"

#include "OrthoTerrain/resources.hpp"

//map yaml format

//map:
// tilesets:
//  - [0, tilesetname]
//  - [4, othertileset]
//
// terrain_width: 7
// tiles: [1,2,3,4,4,4]

namespace tiles
{
	MapSaveData CreateMapSaveData(const MapData& terrain)
	{
		MapSaveData save;

		assert(std::get<0>(terrain).size() % std::get<1>(terrain) == 0);

		save.map_width = std::get<tile_count_t>(terrain);

		//a list of tilesets, to tilesets starting id
		std::vector<std::pair<const resources::tileset*, tile_count_t>> tilesets;

		//for each tile, add its tileset to the tilesets list, 
		// and record the highest tile_id used from that tileset.
		const auto &tiles = std::get<TileArray>(terrain);
		save.tiles.reserve(tiles.size());
		for (const auto &t : tiles)
		{
			//the target tileset will be stored here
			const resources::tileset* tset = nullptr;
			tile_count_t tileset_start_id = 0;
			tile_count_t tile_id = 0;
			//check tilesets cache for id
			for (const auto &s : tilesets)
			{
				const auto &set_tiles = s.first->tiles;
				for (std::vector<tile>::size_type i = 0; i < set_tiles.size(); ++i)
				{
					if (set_tiles[i] == t)
					{
						tset = s.first;
						tileset_start_id = s.second;
						tile_id = i;
						break;
					}
				}

				//if we found it then break
				if (tset)
					break;
			}

			//if we didn't have the tileset already, check the master tileset list.
			if (!tset)
			{
				//check Tilesets for the containing tileset
				for (const auto &s_id : resources::Tilesets)
				{
					const auto s = hades::data::Get<resources::tileset>(s_id);
					const auto &set_tiles = s->tiles;
					for (std::vector<tile>::size_type i = 0; i < set_tiles.size(); ++i)
					{
						if (set_tiles[i] == t)
						{
							if (tilesets.empty())
								tileset_start_id = 0;
							else
							{
								const auto &back = tilesets.back();
								tileset_start_id = back.second + set_tiles.size();
							}

							tset = s;
							tilesets.push_back({ s, tileset_start_id});
							tile_id = i;
							break;
						}
					}

					if (tset)
						break;
				}
			}

			//by now we should have found the tileset and tile id, 
			//if not then saving is impossible
			assert(tset);
			//add whatever was found
			save.tiles.push_back(tile_id + tileset_start_id);
		}

		for (const auto &s : tilesets)
			save.tilesets.push_back({ hades::data::GetAsString(s.first->id),
				s.second });

		return save;
	}

	MapData CreateMapData(const MapSaveData& save)
	{
		MapData out;
		std::get<tile_count_t>(out) = save.map_width;

		using Tileset = std::tuple<const resources::tileset*, tile_count_t>;
		std::vector<Tileset> tilesets;

		//get all the tilesets
		for (const auto &t : save.tilesets)
		{
			auto tileset_name = std::get<hades::types::string>(t);
			auto start_id = std::get<tile_count_t>(t);

			auto tileset_id = hades::data::GetUid(tileset_name);
			auto tileset = hades::data::Get<resources::tileset>(tileset_id);
			assert(tileset);
			tilesets.push_back({ tileset, start_id });
		}

		auto &tiles = std::get<TileArray>(out);
		tiles.reserve(save.tiles.size());
		//for each tile, get it's actual tile value
		for (const auto &t : save.tiles)
		{
			Tileset tset = { nullptr, 0 };
			for(const auto &s : tilesets)
			{
				if (t >= std::get<tile_count_t>(s))
					tset = s;
				else
					break;
			}

			auto tileset = std::get<const resources::tileset*>(tset);
			auto count = std::get<tile_count_t>(tset);
			assert(tileset);
			count = t - count;
			assert(tileset->tiles.size() > count);
			tiles.push_back(tileset->tiles[count]);
		}

		assert(save.tiles.size() == std::get<TileArray>(out).size());

		return out;
	}

	//MapSaveData str format

	//ortho-terrain:
	// tilesets:
	//  - [tilesetname, 0]
	//  - [othertileset, 4]
	//
	// terrain_width: 7
	// tiles: [1,2,3,4,4,4]

	const auto tile_map_keyname = "tile-map";
	const auto tilesets_keyname = "tilesets";
	const auto width_keyname = "width";
	const auto tiles_keyname = "tiles";
	const auto version_keyname = "version";

	YAML::Node GetNode(const YAML::Node &target, hades::types::string name)
	{
		auto node = target[name];
		if (!node)
		{
			auto message = "Missing YAML node: " + name;
			throw tile_map_exception(message.c_str());
		}

		return node;
	}

	//Load function for version: 0
	//throws yaml exception
	MapSaveData LoadVersion0(const YAML::Node &root)
	{
		MapSaveData output;
		//search for the ortho-terrain key
		auto ortho_node = GetNode(root, tile_map_keyname);

		//get the tilesets
		auto tilesets_node = GetNode(ortho_node, tilesets_keyname);

		if (!tilesets_node.IsSequence())
			throw tile_map_exception("Tileset node must be a sequence");

		for (const auto &t : tilesets_node)
		{
			if (!t.IsSequence())
				throw tile_map_exception("Tileset element must be a sequence");

			if (t.size() != 2)
				throw tile_map_exception("Tileset element must contain a name and starting tile_id value");

			auto id_node = t[0];
			if (!id_node || !id_node.IsScalar())
				throw tile_map_exception("Tileset name not scalar");

			auto tileset_name = id_node.as<hades::types::string>();

			auto start_node = t[1];
			if (!start_node || !start_node.IsScalar())
				throw tile_map_exception("Tileset starting tile_id not scalar");

			auto start_tile = start_node.as<hades::types::uint32>();

			static auto last_tile_id = start_tile;

			if (start_tile < last_tile_id)
				throw tile_map_exception("Tilesets must be organised with increasing tile id's.");

			last_tile_id = start_tile;

			output.tilesets.push_back({ tileset_name, start_tile });
		}

		//get the width
		auto width_node = GetNode(ortho_node, width_keyname);

		if (!width_node.IsScalar())
			throw tile_map_exception("Width not scalar");

		output.map_width = width_node.as<tile_count_t>(0);

		if (output.map_width == 0)
			throw tile_map_exception("Map Width invalid. Was wrong type, or value was 0");

		//get the tile list
		auto tiles_node = GetNode(ortho_node, tiles_keyname);

		if (!tiles_node.IsSequence())
			throw tile_map_exception("Tiles node must conatain a sequence of unsigned integers");

		auto tile_count = tiles_node.size();

		if (tile_count % output.map_width != 0)
			throw tile_map_exception("Not enough tiles to create a rectangular map.");

		output.tiles.reserve(tile_count);

		for (const auto &t : tiles_node)
		{
			if (!t.IsScalar())
				throw tile_map_exception("Tile element must be a scalar unsigned integer");

			output.tiles.push_back(t.as<tile_count_t>(0));
		}

		return output;
	}

	MapSaveData Load(const YAML::Node &root)
	{
		try
		{
			//search for the ortho-terrain key
			auto ortho_node = GetNode(root, tile_map_keyname);

			//get the tilesets
			auto version_node = GetNode(ortho_node, version_keyname);
			version_t version;
			if (version_node.IsDefined() && yaml_error(tile_map_keyname, "n/a", version_keyname, "scalar", hades::data::UniqueId::Zero, version_node.IsScalar()))
				version = version_node.as<version_t>(max_supported_version + 1);

			if (version > max_supported_version)
				throw tile_map_exception("Unable to read map, version string higher than supported level");

			return LoadVersion0(root);
		}
		catch (YAML::Exception &e)
		{
			auto message = "Exception parsing YAML: " + hades::types::string(e.what());
			throw tile_map_exception(message.c_str());
		}
	}

	YAML::Emitter &Save(const MapSaveData& obj, YAML::Emitter &target )
	{
		//add terrain header
		//ortho-terrain:
		target << YAML::Key << tile_map_keyname;
		//begin writing the map data
		target << YAML::Value << YAML::BeginMap;

		//version header
		target << YAML::Key << version_keyname;
		target << YAML::Value << 0;

		//add tilesets
		target << YAML::Key << tilesets_keyname;
		target << YAML::Value << YAML::BeginSeq;
		//secquence of tilesets
		//contain name and start id
		for (const auto &set : obj.tilesets)
		{
			target << YAML::Flow << YAML::BeginSeq;
			target << std::get<0>(set) << std::get <1>(set);
			target << YAML::EndSeq;
		}
		//end tilesets
		target << YAML::EndSeq;

		//add terrain_width
		target << YAML::Key << width_keyname;
		target << YAML::Value << obj.map_width;

		//add tiles
		target << YAML::Key << tiles_keyname;
		target << YAML::Value << YAML::Flow << YAML::BeginSeq;

		//tilesequence
		for (auto t : obj.tiles)
			target << t;
		//end tiles sequence
		target << YAML::EndSeq;
		//end terrain map
		target << YAML::EndMap;

		return target;
	}

	YAML::Emitter &operator<<(YAML::Emitter &lhs, const MapSaveData &rhs)
	{
		return Save(rhs, lhs);
	}
}