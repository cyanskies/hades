#ifndef TILES_EDITOR_HPP
#define TILES_EDITOR_HPP

#include "Hades/State.hpp"
#include "Hades/Types.hpp"

#include "Objects/editor.hpp"

#include "Tiles/resources.hpp"
#include "Tiles/tiles.hpp"

//a simple tile editor
//this allows the creation of tile levels with objects placed on them

namespace tiles
{
	namespace editor
	{
		const hades::types::string tile_editor_layout = "editor-layout",
			tile_selector_panel = "tile-selector";

		//screen view height
		// set editor_height to override this
		const hades::types::int32 view_height = 240;

		enum EditMode : objects::editor::EditMode_t
		{TILE = objects::editor::EditMode::OBJECT_MODE_END, TILE_MODE_END};

		enum class TileEditMode {
			NONE, //no tile selected
			TILE, //draw a specific tile, no cleanup
		};

		enum class TileGenerator {
			FILL
		};
	}

	class tile_editor : public objects::object_editor
	{
	public:
		void init() override;
		void draw(sf::RenderTarget &target, sf::Time deltaTime) override;

	protected:
		void FillToolBar(AddToggleButtonFunc, AddButtonFunc, AddSeperatorFunc) override;

		void GenerateDrawPreview(const sf::RenderTarget&, MousePos) override;
		void OnModeChange(EditMode_t t) override;
		void OnClick(const sf::RenderTarget&, MousePos) override;
		//add selector for default tile or generator
		void NewLevelDialog() override;
		void NewLevel() override;
		void SaveLevel() const override;
		void LoadLevel() override;

		void SaveTiles();
		void LoadTiles();

		void DrawPreview(sf::RenderTarget &target) const override;

		//New Map Settings
		editor::TileGenerator TileGenerator = editor::TileGenerator::FILL;

		//core map variables
		MutableTileMap Map;
		const resources::tile_settings *TileSettings = nullptr;

	private:
		void _addTilesToUi();
		void _addTiles(const std::vector<tile> &tiles);
		void _setCurrentTile(tile t);

		//gui
		sfg::Box::Ptr _tileWindow = nullptr;

		//preview drawing variables
		sf::Vector2i _tilePosition;
		draw_size_t _tileDrawSize = 1;
		MutableTileMap _tilePreview;
		editor::TileEditMode _tileMode = editor::TileEditMode::NONE;

		//selected tile for drawing
		//also holds the default for filling a new map
		tile _tileInfo;
	};
}

#endif // !TILES_EDITOR_HPP
