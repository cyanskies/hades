#ifndef HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP
#define HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP

#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/level_editor_component.hpp"
#include "hades/types.hpp"

namespace hades
{
	//provides an interface for editing level name, properties, background colour
	//TODO: background image(stretch, repeat, paralax)
	class level_editor_level_props final : public level_editor_component
	{
	public:
		void level_load(const level&) override;
		void gui_update(gui&, editor_windows&) override;
		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) const override;

	private:
		bool _details_window = false;
		sf::RectangleShape _background;
		string _level_name, _new_name;
		string _level_desc, _new_desc;
	};
}

#endif //!HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP
