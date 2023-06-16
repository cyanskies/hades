#ifndef HADES_OBJECT_INSPECTOR_HPP
#define HADES_OBJECT_INSPECTOR_HPP

#include "hades/debug.hpp"
#include "hades/game_state.hpp"
#include "hades/object_editor.hpp"

namespace hades::debug
{
	template<typename GameSystem>
	struct object_inspector_data
	{
		using object_t = game_obj*;
		using object_ref_t = object_ref;

		bool valid_ref(object_ref_t ref) const noexcept
		{
			return hades::state_api::is_object_stale(ref);
		}

		object_t get_object(object_ref_t ref) const noexcept
		{
			return hades::state_api::get_object_ptr(ref, *extra);
		}

		object_ref_t get_ref(object_t o) const noexcept
		{
			return object_ref_t{ o->id, o };
		}

		string get_name(const object_t o) const
		{
			auto ref = get_ref(o);
			return hades::state_api::get_name(ref, hades::time_point::max(), *game);
		}

		string get_type_name(const object_t o) const
		{
			return o->object_type ? data::get_as_string(o->object_type->id) : to_string(o->id);
		}

		string get_id_string(const object_t o) const
		{
			return to_string(o->id);
		}

		bool has_curve(const object_t o, const resources::curve* c) const noexcept
		{
			return resources::object_functions::has_curve(*o->object_type, c->id);
		}

		auto objects_begin()
		{
			return std::begin(extra->objects);
		}

		auto objects_end()
		{
			return std::end(extra->objects);
		}
		
		// needed for game props
		game_state* game = {};
		// needed for object lookup
		extra_state<GameSystem>* extra = {};

		static constexpr bool visual_editor = true;
		static constexpr object_ref_t nothing_selected = hades::curve_types::bad_object_ref;
	};

	template<typename GameSystem>
	class object_inspector : public hades::debug::basic_overlay
	{
	public:
		object_inspector(game_state* gs, extra_state<GameSystem>* es)
			: _data{ gs, es }, _editor_ui{ &_data }
		{
			assert(_data.game);
			assert(_data.extra);
			return;
		}

		void set_selected(hades::object_ref o) noexcept
		{
			_editor_ui.set_selected(o);
			return;
		}

		void make_sticky(bool b) noexcept
		{
			_sticky = b;
			return;
		}

		bool sticky() const noexcept
		{
			return _sticky;
		}

		hades::object_ref selected() const noexcept
		{
			return _editor_ui.selected();
		}

		void update(gui& g) override
		{
			using namespace std::string_view_literals;

			if (g.window_begin("object inspector"sv, _open))
			{
				if (g.collapsing_header("properties"sv))
				{
					_editor_ui.object_properties(g);
				}
				
				if (g.collapsing_header("object list"sv))
				{
					_editor_ui.show_object_list_buttons(g); 
					_editor_ui.object_list_gui(g);
				}
			}
			g.window_end();

			return;
		}

	private:
		object_inspector_data<GameSystem> _data;
		hades::object_editor_ui<object_inspector_data<game_system>> _editor_ui;
		bool _sticky = false;
		bool _open = true;
	};
}

#endif // !HADES_OBJECT_INSPECTOR_HPP
