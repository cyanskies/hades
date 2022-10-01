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
		void init() override;
		bool handle_event(const event&) override;
		void update(time_duration, const sf::RenderTarget&, input_system::action_set) override;
		void draw(sf::RenderTarget&, time_duration) override;
		void reinit() override;

	private:
		void _close_mod(data::data_manager&);
		void _save_mod(data::data_manager&);
		void _mod_properties(gui&, data::data_manager&);
		void _set_loaded_mod(std::string_view mod, data::data_manager&);
		void _set_mod(std::string_view mod, data::data_manager&);

		struct new_mod_window
		{
			string name = "new_mod";
			bool exists = false;
			bool open = false;
		};

		struct load_mod_window
		{
			string name = "mod";
			bool open = false;
		};

		struct save_mod_window
		{
			string name = "mod";
			bool exists = false;
			bool open = false;
		};

		struct edit_mod_window
		{
			std::vector<std::pair<string, unique_id>> dependencies;
			std::size_t dep_selected = {};
			std::size_t mods_selected = {};
			string pretty_name;
			bool open = false;
		};

		enum class edit_mode
		{
			normal,// for creating or loading a mod on top of the normal game
			already_loaded // for editing one of the mods that was loaded on startup
		};

		sf::View _gui_view;
		gui _gui = { "mod_editor.ini" };
		new_mod_window _new_mod = {};
		load_mod_window _load_mod = {};
		edit_mod_window _edit_mod_window = {};
		save_mod_window _save_as_window = {};
		std::vector<string> _mods;
		unique_id _current_mod = unique_zero;

		hades::data::basic_resource_inspector _inspector;
		sf::RectangleShape _backdrop;
		edit_mode _mode = edit_mode::normal;
		std::size_t _mod_count = {};
	};

	// provide mission editor
	// and additional resource editors
	class basic_mod_editor;
}

#endif //! HADES_MOD_EDITOR_HPP
