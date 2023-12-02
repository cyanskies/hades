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
	template<debug::is_resource_inspector ResourceInspector>
	class mod_editor_impl : public state
	{
	public:
		mod_editor_impl();

		void init() override;
		bool handle_event(const event&) override;
		void update(time_duration, const sf::RenderTarget&, input_system::action_set) override;
		void draw(sf::RenderTarget&, time_duration) override;
		void reinit() override;

		using create_resource_function =           /*     Manager           Id                 Mod Id        */
			std::function<resources::resource_base*(data::data_manager&, unique_id, unique_id)>;

		struct new_resource_window
		{
			string name = "new_resource";
			std::size_t type_index = {};
			bool exists = false;
			bool open = false;
		};

	protected:
		void register_resource_type(std::string type, create_resource_function func);

	private:
		void _close_mod(data::data_manager&);
		void _close_windows();
		void _create_new_resource(gui&, data::data_manager&);
		void _save_mod(data::data_manager&); // deprecated
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

		ResourceInspector _inspector;
		sf::RectangleShape _backdrop;
		sf::View _gui_view;
		unordered_map_string<create_resource_function> _create_resource_funcs;
		gui _gui = { "mod_editor.ini" };
		new_mod_window _new_mod = {};
		load_mod_window _load_mod = {};
		edit_mod_window _edit_mod_window = {};
		save_mod_window _save_as_window = {};
		new_resource_window _new_res_window = {};
		std::vector<string> _mods;
		unique_id _current_mod = unique_zero;
		std::size_t _mod_count = {};
		edit_mode _mode = edit_mode::normal;
	};

	// provide mission editor
	// and additional resource editors
	using basic_mod_editor = mod_editor_impl<hades::data::basic_resource_inspector>;
}

#include "hades/detail/mod_editor.inl"

#endif //! HADES_MOD_EDITOR_HPP
