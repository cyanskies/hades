//#include "OrthoTerrain/terrain.hpp"
//
//#include <array>
//
//#include "SFML/Graphics/RenderTarget.hpp"
//
//#include "yaml-cpp/yaml.h"
//
//#include "Hades/Data.hpp"
//#include "Hades/Utility.hpp"
//
//#include "OrthoTerrain/utility.hpp"
//#include "OrthoTerrain/resources.hpp"
//
//#include "Tiles/tiles.hpp"
//
//namespace ortho_terrain
//{
//	void EnableTerrain(hades::data::data_system *d)
//	{		
//		//create the empty terrainset before letting the tiles resources be registered
//		//this way we replace the tileset for empty terrains with the terrain id
//		CreateEmptyTerrain(d);
//		tiles::EnableTiles(d);
//		RegisterOrthoTerrainResources(d);
//	}
//
//	std::array<bool, 4> TerrainInCorner(tiles::tile t, const resources::terrain *ter)
//	{
//		assert(ter);
//		const auto t_type = GetTransitionType(t, ter);
//		//top left
//		const auto tl = IsTerrainInCorner(Corner::TOP_LEFT, t_type);
//		//top right
//		const auto tr = IsTerrainInCorner(Corner::TOP_RIGHT, t_type);
//		//bottom left
//		const auto bl = IsTerrainInCorner(Corner::BOTTOM_LEFT, t_type);
//		//bottom right
//		const auto br = IsTerrainInCorner(Corner::BOTTOM_RIGHT, t_type);
//
//		return { tl, tr, bl, br };
//	}
//
//	std::vector<const resources::terrain*> &CalculateVertexLayer(const tiles::TileArray &layer, TerrainVertex &out, const resources::terrain *terrain, tiles::tile_count_t width)
//	{
//		const auto tile_width = width - 1;
//		for (std::size_t i = 0u; i < layer.size(); ++i)
//		{
//			const auto corners = TerrainInCorner(layer[i], terrain);
//
//			const auto t_x = i % tile_width;
//			const auto t_y = i / tile_width;
//
//			const auto tl = tiles::FlatPosition({t_x, t_y}, width);
//			const auto bl = tiles::FlatPosition({ t_x, t_y + 1 }, width);
//
//			if (corners[0])
//				out[tl] = terrain;
//			if (corners[1])
//				out[tl + 1] = terrain;
//			if (corners[2])
//				out[bl] = terrain;
//			if (corners[3])
//				out[bl + 1] = terrain;
//		}
//
//		return out;
//	}
//
//	std::size_t VertWidth(std::size_t tiles_width)
//	{
//		return ++tiles_width;
//	}
//
//	TerrainVertex AsTerrainVertex(const std::vector<tiles::TileArray> &map, const std::vector<const resources::terrain*> &terrainset, tiles::tile_count_t width)
//	{
//		assert(!map.empty());
//		if (map[0].size() % width != 0)
//			throw exception("immutable_tile_map is not rectangular, map width: " + hades::to_string(width) + ", tile_count: " + hades::to_string(map[0].size()));
//
//		const auto coloumns = map[0].size() / width;
//
//		static const auto empty_terrain = hades::data::get<resources::terrain>(resources::EmptyTerrainId);
//
//		//a vert array includes a single extra column and row
//		const auto vert_width = VertWidth(width);
//		const auto vert_length = (coloumns + 1) * vert_width;
//		TerrainVertex verts{ vert_length, empty_terrain };
//		assert(map.size() <= terrainset.size());
//		for (std::size_t i = 0u; i < map.size(); ++i)
//			CalculateVertexLayer(map[i], verts, terrainset[i], vert_width);
//
//		return verts;
//	}
//
//	//returns the terrain the tile is sourced from
//	const resources::terrain *FindTerrain(tiles::tile t, const std::vector<const resources::terrain*> &terrain_set)
//	{
//		for (const auto &terr : terrain_set)
//		{
//			if (std::any_of(std::begin(terr->tiles), std::end(terr->tiles), [t](const auto &tile)
//			{
//				return t == tile;
//			}))
//				return terr;
//		}
//
//		return nullptr;
//	}
//
//	//finds out which terrain the provided layer is for
//	const resources::terrain *FindTerrain(const tiles::TileArray &layer, const std::vector<const resources::terrain*> &terrain_set)
//	{
//		//terrain set should not contain the empty terrain
//		assert(std::none_of(std::begin(terrain_set), std::end(terrain_set), [](const auto &terrain) {
//			return terrain->id == resources::EmptyTerrainId;
//		}));
//
//		for (const auto &t : layer)
//		{
//			if (const auto *terrain = FindTerrain(t, terrain_set); terrain)
//				return terrain;
//		}
//
//		//terrain wasn't in terrain_set, check if it's empty_terrain
//		const auto empty_terrain = hades::data::get<resources::terrain>(resources::EmptyTerrainId);
//		for (const auto &t : layer)
//		{
//			if (const auto *terrain = FindTerrain(t, { empty_terrain }); terrain)
//				return terrain;
//		}
//
//		throw exception("Unable to find terrain");
//	}
//
//	bool Within(sf::Vector2u pos, std::size_t target_size, tiles::tile_count_t width)
//	{
//		const auto flat_pos = tiles::FlatPosition(static_cast<sf::Vector2u>(pos), width);
//		return flat_pos < target_size &&
//			pos.x < static_cast<int>(width);
//	}
//	
//	TerrainVertex CalculateLayerVertex(const TerrainVertex &verts, const resources::terrain *t, std::vector<const resources::terrain*> friendly)
//	{
//		static const auto empty = hades::data::get<resources::terrain>(resources::EmptyTerrainId);
//		TerrainVertex v{ verts.size(), empty };
//
//		for (std::size_t i = 0; i < verts.size(); ++i)
//		{
//			const auto terrain = verts[i];
//			if (terrain == t
//				|| std::any_of(std::begin(friendly), std::end(friendly), [terrain](const auto &t) {return terrain == t; }))
//				v[i] = t;
//		}
//
//		return v;
//	}
//
//	//draws vertecies in the vertexmap
//	void ReplaceVertexes(TerrainVertex &verts, tiles::tile_count_t width, const resources::terrain *t, sf::Vector2i pos, tiles::draw_size_t size)
//	{
//		const auto positions = tiles::AllPositions(pos, size);
//		const auto vert_width = VertWidth(width);
//		//for each position, update the vertex
//		for (const auto &pos : positions)
//		{
//			if (Within(static_cast<sf::Vector2u>(pos), verts.size(), vert_width))
//			{
//				const auto f_pos = tiles::FlatPosition(static_cast<sf::Vector2u>(pos), vert_width);
//				verts[f_pos] = t;
//			}
//		}
//	}
//
//	void UpdateArray(tiles::TileArray &arr, const TerrainVertex &verts, tiles::tile_count_t vertex_width,
//		std::vector<const resources::terrain*> t_list, sf::Vector2i pos, tiles::draw_size_t size)
//	{
//		static const auto empty_terrain = hades::data::get<resources::terrain>(resources::EmptyTerrainId);
//		static const auto empty_tile = tiles::GetEmptyTile();
//		
//		//for each of the positions that have been updated
//		const auto positions = tiles::AllPositions(pos, size + 1);
//		for (const auto &pos : positions)
//		{
//			//ensure the position is within the limits of the map
//			if (!Within(static_cast<sf::Vector2u>(pos), arr.size(), vertex_width - 1))
//				continue;
//
//			//get the terrain in each corner of this tile
//			const auto corners = GetCornerData(static_cast<sf::Vector2u>(pos), verts, vertex_width);
//			const auto flat_pos = tiles::FlatPosition(static_cast<sf::Vector2u>(pos), vertex_width - 1);
//
//			//if none of the corners are the current layers terrain, then just dump an empty
//			if (std::none_of(std::begin(corners), std::end(corners), [t = t_list.front()](const auto &terrain)
//			{
//				return t == terrain;
//			}))
//			{
//				arr[flat_pos] = empty_tile;
//				continue;
//			}
//
//			//build a bool array of empty corners, we use this to pick the transition tile
//			std::array<bool, 4> empty_corners{ false };
//			for (std::size_t k = 0; k < corners.size(); ++k)
//			{
//				const auto found = std::any_of(std::begin(t_list), std::end(t_list),
//					[t = corners[k]](const auto terrain) {
//					return terrain == t;
//				});
//					
//				empty_corners[k] = !found;
//			}
//
//			//apply the correct transition
//			const auto type = PickTransition(empty_corners);
//			//if every corner is empty, then place an empty tile
//			if (type == transition2::NONE)
//				arr[flat_pos] = empty_tile;
//			else
//			{
//				//apply the selected transition
//				const auto tile_list = GetTransitionConst(type, *t_list.front());
//				const auto tile = RandomTile(tile_list);
//				arr[flat_pos] = tile;
//			}
//		}
//	}
//
//	void UpdateTileArrays(TerrainMapData &map, const TerrainVertex &verts, tiles::tile_count_t vertex_width,
//		std::vector<const resources::terrain*> terrainset, sf::Vector2i pos, tiles::draw_size_t size)
//	{
//		for (std::size_t i = 0; i < terrainset.size(); ++i)
//		{
//			std::vector<const resources::terrain*> terrain_list;
//			std::copy(std::begin(terrainset) + i, std::end(terrainset), std::back_inserter(terrain_list));
//			UpdateArray(map.tile_map_stack[i], verts, vertex_width, terrain_list, pos, size);
//		}
//	}
//
//	void ReplaceTerrain(TerrainMapData &map, TerrainVertex &verts, tiles::tile_count_t width,
//		const resources::terrain *t, sf::Vector2i pos, tiles::draw_size_t size)
//	{
//		ReplaceVertexes(verts, width, t, pos, size );
//		const auto v_width = VertWidth(width);
//		UpdateTileArrays(map, verts, v_width, map.terrain_set, pos, size + 1);
//	}
//
//	////////////////
//	// TerrainMap //
//	////////////////
//
//	TerrainMap::TerrainMap(const TerrainMapData &dat, tiles::tile_count_t width)
//	{
//		create(dat, width);
//	}
//
//	void TerrainMap::create(const TerrainMapData &map, tiles::tile_count_t width)
//	{
//		if (map.terrain_set.size() != map.tile_map_stack.size())
//			throw exception("Map malformed, should contain a layer for each terrain in terrainset");
//
//		for (const auto &l : map.tile_map_stack)
//			Map.emplace_back(tiles::immutable_tile_map({ l, width }));
//	}
//
//	void TerrainMap::draw(sf::RenderTarget& target, sf::RenderStates states) const
//	{
//		for (const auto &l : Map)
//			l.draw(target, states);
//	}
//	
//	sf::FloatRect GetBounds(const std::vector<const tiles::immutable_tile_map*> &map)
//	{
//		if (map.empty())
//			return sf::FloatRect{};
//		else
//		{
//			sf::FloatRect r{};
//			for (const auto &l : map)
//			{
//				const auto rect = l->getLocalBounds();
//				r.left = std::min(r.left, rect.left);
//				r.top = std::min(r.top, rect.top);
//				r.width = std::max(r.width, rect.width);
//				r.height = std::max(r.height, rect.height);
//			}
//
//			return r;
//		}
//	}
//
//	sf::FloatRect TerrainMap::getLocalBounds() const
//	{
//		std::vector<const tiles::immutable_tile_map*> map;
//		for (const auto &layer : Map)
//			map.emplace_back(&layer);
//
//		return GetBounds(map);
//	}
//
//	///////////////////////
//	// MutableTerrainMap //
//	///////////////////////
//
//	MutableTerrainMap::MutableTerrainMap(const TerrainMapData &dat, tiles::tile_count_t width)
//	{
//		create(dat, width);
//	}
//
//	void MutableTerrainMap::create(const TerrainMapData &map, tiles::tile_count_t width)
//	{
//		tiles::tile_count_t size = 0u;
//		for (const auto &layer : map.tile_map_stack)
//			size = std::max(size, layer.size());
//
//		std::vector<tile_layer> new_map;
//
//		if (map.tile_map_stack.size() > map.terrain_set.size())
//			throw exception("terrain map is malformed, contains more tile layers than terrain types");
//		
//		//ensure the map is well formed, and patch it up if needed
//		const auto empty_tile = tiles::GetEmptyTile();
//		const auto &empty_terrain = resources::EmptyTerrainId;
//		tiles::TileArray empty_layer{ size, empty_tile };
//		std::size_t i = 0u, j = 0u;
//
//		for (std::size_t i = 0; i < map.tile_map_stack.size() && i < map.terrain_set.size(); ++i)
//			new_map.emplace_back(map.terrain_set[i], map.tile_map_stack[i]);
//
//		std::vector<tiles::TileArray> tile_array;
//		for (const auto &arr : new_map)
//			tile_array.emplace_back(std::get<tiles::TileArray>(arr));
//
//		//generate vertex map
//		auto verts = AsTerrainVertex(tile_array, map.terrain_set, width);
//
//		_terrain_layers.clear();
//		for (const auto &tiles : new_map)
//			_terrain_layers.emplace_back(tiles::mutable_tile_map{ {std::get<tiles::TileArray>(tiles), width} });
//
//		std::swap(new_map, _tile_layers);
//		std::swap(verts, _vdata);
//		_width = width;
//
//		//apply colour if it's already been set
//		setColour(_colour);
//	}
//
//	void MutableTerrainMap::create(std::vector<const resources::terrain*> terrainset, const resources::terrain *terr, tiles::tile_count_t width, tiles::tile_count_t height)
//	{
//		_terrain_layers.clear();
//		_tile_layers.clear();
//		_colour = sf::Color::White;
//		_vdata.clear();
//
//		const auto size = width * height;
//
//		//ensure the map is well formed, and patch it up if needed
//		const auto empty_tile = tiles::GetEmptyTile();
//		const auto &empty_terrain = resources::EmptyTerrainId;
//		tiles::TileArray empty_layer{ size, empty_tile };
//		assert(std::find(std::begin(terrainset), std::end(terrainset), terr) != std::end(terrainset));
//		assert(!terr->full.empty());
//		tiles::TileArray full_layer{ size, terr->full.front() };
//
//		_width = width;
//		std::vector<tiles::TileArray> t_array;
//
//		for (const auto &t : terrainset)
//		{
//			if (t == terr)
//			{
//				_tile_layers.emplace_back(t, full_layer);
//				t_array.emplace_back(full_layer);
//			}
//			else
//			{
//				_tile_layers.emplace_back(t, empty_layer);
//				t_array.emplace_back(empty_layer);
//			}
//		}
//
//		//generate vertex map
//		_vdata = AsTerrainVertex(t_array, terrainset, width);
//		assert(std::all_of(std::begin(_vdata), std::end(_vdata), [t=terr](const auto &terrain)
//		{
//			return t == terrain; }
//		));
//
//		for (const auto &tiles : t_array)
//			_terrain_layers.emplace_back(tiles::mutable_tile_map{ {tiles, width} });
//
//		//apply colour if it's already been set
//		setColour(_colour);
//	}
//
//	void MutableTerrainMap::draw(sf::RenderTarget& target, sf::RenderStates states) const
//	{	
//		for (const auto &layer : _terrain_layers)
//			layer.draw(target, states);
//	}
//
//	sf::FloatRect MutableTerrainMap::getLocalBounds() const
//	{
//		std::vector<const tiles::immutable_tile_map*> map;
//		for (const auto &layer : _terrain_layers)
//			map.emplace_back(&layer);
//
//		return GetBounds(map);
//	}
//
//	void MutableTerrainMap::replace(const resources::terrain *t, const sf::Vector2i &position, tiles::draw_size_t amount)
//	{
//		TerrainMapData map = getMap();
//		ReplaceTerrain(map, _vdata, _width, t, position, amount);
//
//		for (std::size_t i = 0u; i < map.tile_map_stack.size(); ++i)
//		{
//			_terrain_layers[i].update({ map.tile_map_stack[i], _width });
//			std::swap(std::get<tiles::TileArray>(_tile_layers[i]), map.tile_map_stack[i]);
//		}
//	}
//
//	void MutableTerrainMap::setColour(sf::Color c)
//	{
//		_colour = c;
//
//		for (auto &l : _terrain_layers)
//			l.setColour(c);
//	}
//
//	TerrainMapData MutableTerrainMap::getMap() const
//	{
//		TerrainMapData map;
//		for (const auto &[terrain, tiles] : _tile_layers)
//		{
//			map.terrain_set.emplace_back(terrain);
//			map.tile_map_stack.emplace_back(tiles);
//		}
//
//		return map;
//	}
//
//	TerrainMapData as_terraindata(const terrain_layer &l, tiles::tile_count_t width)
//	{
//		TerrainMapData m;
//
//		for (const auto &n : l.terrainset)
//		{
//			const auto id = hades::data::get_uid(n);
//			const auto t = hades::data::get<resources::terrain>(id);
//			m.terrain_set.emplace_back(t);
//		}
//
//		for (const auto &t : l.tile_layers)
//		{
//			std::vector<tiles::TileSetInfo> tilesets;
//			for (const auto &set : t.tilesets)
//			{
//				const auto id = hades::data::get_uid(std::get<hades::types::string>(set));
//				tilesets.emplace_back(id, std::get<tiles::tile_count_t>(set));
//			}
//
//			const auto layer_data = tiles::as_mapdata({tilesets, t.tiles, width});
//			m.tile_map_stack.emplace_back(std::get<tiles::TileArray>(layer_data));
//		}
//
//		return m;
//	}
//
//	terrain_layer as_terrain_layer(const TerrainMapData &m, tiles::tile_count_t width)
//	{
//		terrain_layer l;
//		for (const auto &t : m.terrain_set)
//			l.terrainset.emplace_back(hades::data::get_as_string(t->id));
//
//		for (const auto &layer : m.tile_map_stack)
//		{
//			const auto tiles = tiles::as_rawmap({ layer, width });
//
//			std::vector<std::tuple<hades::types::string, tiles::tile_count_t>> tilesets;
//			for (const auto &set : std::get<std::vector<tiles::TileSetInfo>>(tiles))
//			{
//				const auto name = hades::data::get_as_string(std::get<hades::unique_id>(set));
//				tilesets.emplace_back(name, std::get<tiles::tile_count_t>(set));
//			}
//
//			l.tile_layers.emplace_back(tiles::tile_layer{ tilesets, std::get<std::vector<tiles::tile_count_t>>(tiles) });
//		}
//
//		return l;
//	}
//
//	constexpr auto terrain_yaml = "terrain";
//	constexpr auto tile_yaml = "tile-layers";
//	constexpr auto terrainset_yaml = "terrainset";
//	
//	void ReadTerrainFromYaml(const YAML::Node &n, level &target)
//	{
//		//level root:
//		//    terrain:
//		//        terrainset: [t1, t2, t3, t4 ,t5]
//		//        tile-layers:
//		//            //tile_later layout
//
//		const auto terrain_n = n[terrain_yaml];
//
//		//TODO: add error logging here
//		if (!terrain_n.IsDefined() || !terrain_n.IsMap())
//			return;
//
//		const auto terrainset_n = terrain_n[terrainset_yaml];
//		if (!terrainset_n.IsDefined() || !terrainset_n.IsSequence())
//			return;
//
//		std::vector<hades::types::string> terrains;
//
//		for (const auto &t : terrainset_n)
//			terrains.emplace_back(t.as<hades::types::string>());
//
//		std::vector<tiles::tile_layer> layers;
//		const auto tile_layers = terrain_n[tile_yaml];
//		if (!tile_layers.IsDefined() || !tile_layers.IsSequence())
//			return;
//
//		for (const auto &l : tile_layers)
//		{
//			tiles::tile_layer t;
//			tiles::ReadTileLayerFromYaml(l, t);
//			layers.emplace_back(t);
//		}
//
//		target.terrain = terrain_layer{ layers, terrains };
//	}
//
//	YAML::Emitter &WriteTerrainToYaml(const level &l, YAML::Emitter &e)
//	{
//		e << YAML::Key << terrain_yaml;
//		e << YAML::Value << YAML::BeginMap;
//		
//		//write terrains
//		e << YAML::Key << terrainset_yaml;
//		e << YAML::Value << YAML::Flow;
//		e << YAML::BeginSeq;
//
//		for (const auto &t : l.terrain.terrainset)
//			e << t;
//
//		e << YAML::EndSeq;
//
//		//write each tile layer
//		e << YAML::Key << tile_yaml;
//		e << YAML::Value << YAML::BeginSeq;
//
//		for (const auto &layer : l.terrain.tile_layers)
//			tiles::WriteTileLayerToYaml(layer, e);
//		
//		e << YAML::EndSeq;
//
//		e << YAML::EndMap;
//		return e;
//	}
//}
