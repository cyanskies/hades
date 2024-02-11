#ifndef HADES_LEVEL_EDITOR_COMPONENT_HPP
#define HADES_LEVEL_EDITOR_COMPONENT_HPP

#include <functional>
#include <tuple>
#include <vector>

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/level.hpp"
#include "hades/math.hpp"
#include "hades/timers.hpp"
#include "hades/types.hpp"
#include "hades/vector_math.hpp"

namespace hades::resources
{
	struct animation;
}

namespace hades::editor::gui_names
{
	using namespace std::string_view_literals;
	//Window name for left toolbox window
	constexpr auto toolbox = "##toolbox"sv;
	constexpr auto new_level = "New Level"sv;
	constexpr auto load_level = "Load Level"sv;
	constexpr auto save_level = "Save Level"sv;
	constexpr auto resize_level = "Resize Level"sv;
}

namespace hades::editor
{
	using namespace std::string_view_literals;
	constexpr auto new_level_name = "A brand new level"sv;
	constexpr auto new_level_description = "An empty level, ready to be filled with content"sv;

	constexpr auto zoom_max = 3.f;
}

namespace hades
{
	class level_editor_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	class new_level_editor_error : public level_editor_error
	{
	public:
		using level_editor_error::level_editor_error;
	};

	class resize_level_editor_error : public level_editor_error
	{
	public:
		using level_editor_error::level_editor_error;
	};

	class gui;
	struct mission;
	
	class level_editor_component
	{
	public:
		struct editor_windows
		{
			bool new_level = false;
			bool resize_level = false;
			bool load_level = false;
			bool save_level = false;
		};

		virtual ~level_editor_component() noexcept = default;

		//generic callbacks, these are always available
		using activate_brush_f = std::function<void(void)>;
		using get_tags_at_f = std::function<tag_list(rect_float)>;
		using player_reference = std::pair<unique_id, const object_instance*>;
		using get_players_return_type = std::vector<player_reference>;
		using get_players_f = std::function<get_players_return_type(void)>;
		using get_world_rotation_f = std::function<float(void)>;

		template<typename ActivateBrush, typename GetTerrainTagsAt, typename GetObjTagsAt, typename GetPlayers, typename GetWorldRotation>
		void install_callbacks(ActivateBrush ab, GetTerrainTagsAt get_terrain_tags,
			GetObjTagsAt get_obj_tags, GetPlayers get_players, GetWorldRotation get_world_rotate)
		{
			_activate_brush = ab;
			_get_terrain_tags_at = get_terrain_tags;
			_get_object_tags_at = get_obj_tags;
			_get_players = get_players;
			_get_world_rotation = get_world_rotate;
		}

		void activate_brush() noexcept
		{
			std::invoke(_activate_brush);
		}

		get_players_return_type get_players() const
		{
			return std::invoke(_get_players);
		}

		//searches all components
		tag_list get_terrain_tags_at(rect_float r) const
		{
			return std::invoke(_get_terrain_tags_at, r);
		}
		
		//searches all components
		tag_list get_object_tags_at(rect_float r) const
		{
			return std::invoke(_get_object_tags_at, r);
		}

		//searches all components
		tag_list get_tags_at(rect_float r)
		{
			auto a = get_terrain_tags_at(r);
			const auto o = get_object_tags_at(r);

			a.reserve(std::size(a) + std::size(o));
			a.insert(std::end(a), std::begin(o), std::end(o));

			return {};
		}

		float get_world_rotation() const
		{
			return std::invoke(_get_world_rotation);
		}

		//compoenents can throw new_level_editor_error
		// if they with to prevent creation of a new level,
		virtual level level_new(level l) const { return l; }
		virtual void level_load(const level&) {}
		virtual level level_save(level l) const { return l; }
		virtual void level_resize(vector2_int /*new_size*/, vector2_int /*current_offset*/) {}

		virtual void gui_update(gui&, editor_windows&) {}

		//mouse position, in world coords
		using mouse_pos = vector2_float;

		//used to generate info for draw_brush_preview
		virtual void make_brush_preview(time_duration, mouse_pos) {}

		//return any relevent tags for that location
		// this is for components that implement terrain or object editing
		virtual tag_list get_terrain_tags_at_location(rect_float) const { return {}; }
		virtual tag_list get_object_tags_at_location(rect_float) const { return {}; }

		virtual void on_reinit(vector2_float /*window_size*/, vector2_float /*view_size*/) {}

		virtual void on_click(mouse_pos) {}
	
		virtual void on_drag_start(mouse_pos) {}
		virtual void on_drag(mouse_pos) {}
		virtual void on_drag_end(mouse_pos) {}

		virtual void on_world_rotate(float) {}
		virtual void on_screen_move(rect_float /*view_area*/) {}

		// tells the component whether it should render with height on or not
		virtual void on_height_toggle(bool) {}

		// only one component should provide terrain
		virtual const terrain_map* peek_terrain() const 
		{
			return {};
		}

		virtual void draw(sf::RenderTarget&, time_duration, sf::RenderStates) {}

		virtual void draw_brush_preview(sf::RenderTarget&, time_duration, sf::RenderStates) {}

	private:
		activate_brush_f _activate_brush;
		get_tags_at_f _get_terrain_tags_at;
		get_tags_at_f _get_object_tags_at;
		get_players_f _get_players;
		get_world_rotation_f _get_world_rotation;
	};
}

#endif //!HADES_LEVEL_EDITOR_COMPONENT_HPP
