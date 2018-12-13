#ifndef HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP
#define HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP

#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/level_editor_component.hpp"
#include "hades/types.hpp"

// TODO:
// support for background colours, images(stretch, repeat, paralax)

namespace hades
{
	//provides an interface for editing level name, properties, background colour
	class level_editor_level_props final : public level_editor_component
	{
	public:
		void level_load(const level&) override;
		level level_save(level l) const override;

		void gui_update(gui&, editor_windows&) override;
		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override;

	private:
		void _edit_background_window(gui&);

		//TODO: background object
		//TODO: how to clip the background at the edges of the world
		bool _details_window = false;
		bool _background_window = false;
		sf::RectangleShape _background;
		std::vector<level::background_layer> _background_layer_settings;
		string _level_name, _new_name;
		string _level_desc, _new_desc;
	};
}

#endif //!HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP
