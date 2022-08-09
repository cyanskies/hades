#ifndef HADES_MOD_EDITOR_HPP
#define HADES_MOD_EDITOR_HPP

#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/data.hpp"
#include "hades/gui.hpp"
#include "hades/state.hpp"

#include "hades/debug/resource_inspector.hpp"

// mod editor displays the contents of the mod/yaml
// it supplies viewers to examine, modify and create resources of each type

// resource tool: provides a pretty name for  the resource type
//				handles creating and modifying a resource

// mission editor opens missions

// mod editor needs to be able to save the resources back to file

namespace hades
{
	class mod_editor_impl : public state
	{
	public:
		//mod_editor_impl();
		//mod_editor_impl() // make new mod

		void init() override;
		bool handle_event(const event&) override;
		void update(time_duration, const sf::RenderTarget&, input_system::action_set) override;
		void draw(sf::RenderTarget&, time_duration) override;
		void reinit() override;
	private:
		sf::View _gui_view;
		gui _gui;

		hades::data::resource_inspector _inspector;
		sf::RectangleShape _backdrop;
		data::data_manager *d;
	};

	// provide mission editor
	// and additional resource editors
	class basic_mod_editor;
}

#endif //! HADES_MOD_EDITOR_HPP
