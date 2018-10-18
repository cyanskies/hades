#include "Tiles/resources.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/Data.hpp"
#include "Hades/data_system.hpp"

//#include "Tiles/editor.hpp"
#include "Tiles/tiles.hpp"

#include "Tiles/resource/error_tile.hpp"

namespace tiles
{
	using hades::unique_id;

	constexpr auto tile_settings_name = "tile-settings";

	namespace id
	{
		hades::unique_id empty_tile_texture = hades::unique_id::zero;
		hades::unique_id tile_settings = hades::unique_id::zero;
	}

	namespace resources
	{
		void parseTileset(unique_id, const YAML::Node&, hades::data::data_manager*);
	}

	void RegisterTileResources(hades::data::data_system* data)
	{	
		data->register_resource_type("tile-settings", tiles::resources::parseTileSettings);
		data->register_resource_type("tilesets", tiles::resources::parseTileset);

		//create texture for error_tile
		const unique_id error_tile_texture;
		auto error_t_tex = hades::data::FindOrCreate<hades::resources::texture>(error_tile_texture, unique_id::zero, data);
		error_t_tex->loaded = true;
		error_t_tex->value.loadFromMemory(error_tile::data, error_tile::len);

		//create error tileset and add a default error tile
		const hades::unique_id error_tset_id;
		auto error_tset = hades::data::FindOrCreate<resources::tileset>(error_tset_id, unique_id::zero, data);
		const tile error_tile{ error_t_tex, 0, 0 };
		error_tset->tiles.emplace_back(error_tile);

		//create texture for empty tile
		id::empty_tile_texture = hades::unique_id{};
		auto empty_t_tex = hades::data::FindOrCreate<hades::resources::texture>(id::empty_tile_texture, unique_id::zero, data);
		sf::Image empty_i;
		empty_i.create(1u, 1u, sf::Color::Transparent);
		empty_t_tex->value.loadFromImage(empty_i);
		empty_t_tex->value.setRepeated(true);
		empty_t_tex->repeat = true;

		const hades::unique_id empty_tset_id = data->get_uid(empty_tileset_name);
		auto empty_tset = hades::data::FindOrCreate<resources::tileset>(empty_tset_id, unique_id::zero, data);
		const tile empty_tile{ empty_t_tex, 0, 0 };
		empty_tset->tiles.emplace_back(empty_tile);

		resources::Tilesets.emplace_back(empty_tset_id);

		//create default tile settings obj
		id::tile_settings = hades::data::make_uid(tile_settings_name);
		auto settings = hades::data::FindOrCreate<resources::tile_settings>(id::tile_settings, unique_id::zero, data);
		settings->error_tileset = error_tset;
		settings->empty_tileset = empty_tset;
		settings->tile_size = 8;
	}

	bool operator==(const tile &lhs, const tile &rhs)
	{
		return lhs.left == rhs.left &&
			lhs.texture == rhs.texture &&
			lhs.top == rhs.top;
	}

	bool operator!=(const tile &lhs, const tile &rhs)
	{
		return !(lhs == rhs);
	}

	bool operator<(const tile &lhs, const tile &rhs)
	{
		if (lhs.texture != rhs.texture)
			return lhs.texture < rhs.texture;
		else if(lhs.left != rhs.left)
			return lhs.left < rhs.left;
		else
			return lhs.top < rhs.top;
	}

	namespace resources
	{
		std::vector<unique_id> Tilesets;

		void LoadTileset(hades::resources::resource_base *r, hades::data::data_manager *d)
		{
			auto tset = static_cast<tileset*>(r);

			//hot load all the textures used in this tileset
			for (const auto t : tset->tiles)
			{
				if (!t.texture)
				{
					const auto message = "Tile in tileset: " + d->get_as_string(tset->mod) + "::" + d->get_as_string(tset->id) + ", are missing a texture";
					LOGERROR(message);
					throw std::logic_error(message);
				}
				else if (t.texture && !t.texture->loaded)
					d->get<hades::resources::texture>(t.texture->id);
			}
		}

		tileset::tileset() : hades::resources::resource_type<tileset_t>(LoadTileset) {}

		tileset::tileset(hades::resources::resource_type<tileset_t>::loaderFunc func) 
			: hades::resources::resource_type<tileset_t>(func) {}

		void parseTileSettings(unique_id mod, const YAML::Node& node, hades::data::data_manager* data_manager)
		{
			//terrain-settings:
			//    tile-size: 32
			static const hades::types::string resource_type = tile_settings_name;
			static const hades::types::uint8 default_size = 8;
			auto id = data_manager->get_uid(resource_type);

			auto settings = hades::data::FindOrCreate<tile_settings>(id, mod, data_manager);

			if (!settings)
				return;

			settings->tile_size = yaml_get_scalar(node, resource_type, "n/a", "tile-size", mod, settings->tile_size);
			const auto error_tset_id = yaml_get_uid(node, resource_type, "n/a", "error-tileset", mod);
			if (error_tset_id != hades::unique_id::zero)
			{
				const auto error_tset = hades::data::FindOrCreate<tileset>(error_tset_id, mod, data_manager);
				settings->error_tileset = error_tset;
			}
		}

		std::vector<tile> parseTiles(const hades::resources::texture *texture, tile_size_t tile_size, tile_size_t top, tile_size_t left, tile_count_t width, tile_count_t count, const traits_list &traits)
		{
			assert(texture);
			std::vector<tile> out;
			tile_count_t row = 0, column = 0;
			while (count > 0)
			{
				tile t;
				t.texture = texture;
				t.traits = traits;
				t.left = column * tile_size + left;
				t.top = row * tile_size + top;

				if (column < width)
					++column;
				else
				{
					column = 0;
					++row;
				}

				out.push_back(t);
				--count;
			}

			return out;
		}

		std::vector<tile> ParseTileSection(hades::resources::texture *texture, tile_size_t tile_size, YAML::Node &tiles_node,
			hades::types::string resource_type, hades::types::string name, unique_id mod)
		{
			tile_size_t left = yaml_get_scalar<tile_size_t>(tiles_node, resource_type, name, "left", mod, 0),
				top = yaml_get_scalar<tile_size_t>(tiles_node, resource_type, name, "top", mod, 0);
			tile_count_t width = yaml_get_scalar<tile_count_t>(tiles_node, resource_type, name, "tiles_per_row", mod, 0),
				count = yaml_get_scalar<tile_count_t>(tiles_node, resource_type, name, "tile_count", mod, 0);

			auto traits_str = yaml_get_sequence<hades::types::string>(tiles_node, resource_type, name, "traits", mod);
			traits_list traits;

			std::transform(std::begin(traits_str), std::end(traits_str), std::back_inserter(traits), [](hades::types::string s) {
				return hades::data::get_uid(s);
			});

			return parseTiles(texture, tile_size, top, left, width - 1 /* make width 0 based */, count, traits);
		}

		void parseTileset(unique_id mod, const YAML::Node& node, hades::data::data_manager *data)
		{
			//tilesets:
			//    sand: <// tileset name, these must be unique
			//        texture: <// texture to draw the tiles from
			//        tiles: <// a section of tiles
			//            left: <// pixel start of tileset
			//            top: <// pixel left of tileset
			//            tiles_per_row: <// number of tiles per row
			//            tile_count: <// total amount of tiles in tileset;
			//            traits: <// a list of trait tags that get added to the tiles in this tileset

			static const hades::types::string resource_type = "tileset";

			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			for (auto n : node)
			{
				auto namenode = n.first;
				auto name = namenode.as<hades::types::string>();
				auto id = data->get_uid(name);

				auto tset = hades::data::FindOrCreate<tileset>(id, mod, data);

				if (!tset)
					continue;

				tset->mod = mod;

				auto v = n.second;
				auto tex = v["texture"];
				if (!tex.IsDefined())
				{
					//tile set must have a texture
					LOGERROR("Tileset: " + name + ", doesn't define a texture, skipping");
					continue;
				}

				auto texid = data->get_uid(tex.as<hades::types::string>());

				try
				{
					auto tile_settings = GetTileSettings();

					auto tiles_section = v["tiles"];
					if (!tiles_section.IsDefined())
					{
						LOGERROR("Cannot parse tiles section, skipping");
						continue;
					}

					const auto tex = data->get<hades::resources::texture>(texid);

					auto tile_list = ParseTileSection(tex, tile_settings->tile_size, tiles_section, resource_type, name, mod);
					std::move(std::begin(tile_list), std::end(tile_list), std::back_inserter(tset->tiles));
				}
				catch (tile_map_exception&)
				{
					LOGERROR("Cannot load tileset: " + name + ", missing tile-settings resource.");
					continue;
				}

				Tilesets.push_back(id);
			}
		}
	}
}