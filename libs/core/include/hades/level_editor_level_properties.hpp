#ifndef HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP
#define HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP

#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/background.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/types.hpp"

namespace hades
{
	void create_level_editor_properties_variables();

	//provides an interface for editing level name, properties, background colour
	//TODO: set ai input script here too
	//TODO: level save scripts
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

		struct background_pick_window
		{
			bool open = false;
			unique_id resource = unique_id::zero;
			string input;
		};

		struct background_settings_window
		{
			bool open = false;
			std::size_t selected_layer = 0;
			string animation_input;
		};

		level level_new(level l) const override;
		void level_load(const level&) override;
		level level_save(level l) const override;
		void level_resize(vector_int s, vector_int o) override;

		void gui_update(gui&, editor_windows&) override;
		void draw(sf::RenderTarget&, time_duration, sf::RenderStates) override;

	private:
		background_pick_window _background_pick_window;
		background_settings _background_settings;
		background_settings _background_uncommitted;
		background_settings_window _background_window;
		background _background;
		bool _details_window = false;
		string _level_name, _new_name = to_string(editor::new_level_name);
		string _level_desc, _new_desc = to_string(editor::new_level_description);
		unique_id _load_script = unique_zero;
		unique_id _player_input = unique_zero;
		unique_id _ai_input = unique_zero;
	};
}

namespace hades::cvars
{
	using namespace std::string_view_literals;
	constexpr auto editor_player_script_default = "editor_player_script_default"sv;
	constexpr auto editor_load_script_default = "editor_load_script_default"sv;

	namespace default_value
	{
		constexpr auto editor_player_script_default = ""sv;
		constexpr auto editor_load_script_default = ""sv;
	}
}

#endif //!HADES_LEVEL_EDITOR_LEVEL_PROPERTIES_HPP
