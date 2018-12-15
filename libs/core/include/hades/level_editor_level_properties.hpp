#ifndef HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP
#define HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP

#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/background.hpp"
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
		struct background_settings
		{
			struct background_layer
			{
				unique_id anim = unique_id::zero;
				vector_float offset{ 0.f, 0.f };
				vector_float parallax{ 1.f, 1.f };
			};

			colour col = colours::black;
			std::vector<background_layer> layers;
		};

		void level_load(const level&) override;
		level level_save(level l) const override;

		void gui_update(gui&, editor_windows&) override;
		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override;

	private:
		//TODO: background object
		//TODO: how to clip the background at the edges of the world
		background_settings _background_settings;
		background_settings _background_uncommitted;
		background _background;
		bool _details_window = false;
		bool _background_window = false;
		string _level_name, _new_name;
		string _level_desc, _new_desc;
	};
}

#endif //!HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP
