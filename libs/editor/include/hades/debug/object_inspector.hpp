#ifndef HADES_OBJECT_INSPECTOR_HPP
#define HADES_OBJECT_INSPECTOR_HPP

#include <ranges>

#include "hades/debug.hpp"
#include "hades/game_state.hpp"
#include "hades/level_interface.hpp"
#include "hades/object_editor.hpp"

namespace hades::debug
{
	template<typename GameSystem>
	struct object_inspector_data
	{
		using object_t = game_obj*;
		using object_ref_t = object_ref;
		using curve_t = const resources::curve*;

		const resources::object* get_type(object_t o)
		{
			return o->object_type;
		}

		bool valid_ref(object_ref_t ref) const noexcept
		{
			return !hades::state_api::is_object_stale(ref);
		}

		object_t get_object(object_ref_t ref) const noexcept
		{
			return hades::state_api::get_object_ptr(ref, game->get_extras());
		}

		object_ref_t get_ref(object_t o) const noexcept
		{
			return object_ref_t{ o->id, o };
		}

		// we don't support adding objects
		//object_ref_t add(object_instance);

		void remove(object_ref_t ref) noexcept
		{
			game->destroy_object(ref, time);
			return;
		}

		int32 to_int(const object_ref_t o) const noexcept
		{
			return integer_cast<int32>(static_cast<entity_id::value_type>(o.id));
		}

		string get_name(const object_t o) const
		{
			auto ref = get_ref(o);
			return hades::state_api::get_name(ref, hades::time_point::max(), game->get_state());
		}

		bool set_name(object_t o, std::string_view s)
		{
			// NOTE: this function acts differently to the set_name functions 
			// from	game_state.hpp; This function doesn't allow stealing names
			// it also allows removing names.
			const auto ref = get_ref(o);
			auto& g = game->get_state();

			auto remove_name = [&]() {
				// removing current name binding
				for (auto& [str, val] : g.names)
				{
					if (!val.empty() && val.get(time) == ref)
						val.replace_keyframes(time, nothing_selected);
				}
			};

			// remove name
			if (s.empty())
			{
				std::invoke(remove_name);
				return true;
			}

			// find current allocation for this name
			auto iter = g.names.find(s);
			// make a new allocation if one doesn't exist
			if (iter == end(g.names))
				std::tie(iter, std::ignore) = g.names.emplace(to_string(s), step_curve<object_ref>{});

			//find name
			step_curve<object_ref>& value = iter->second;

			if (!value.empty())
			{
				const auto cur = value.get(time);
				// dont steal the name from another object
				if (cur != nothing_selected)
					return false;

				// don't bother updating if the current name holder is us
				if (cur == ref)
					return true;
			}

			// remove our old name, and set the new one
			std::invoke(remove_name);
			value.replace_keyframes(time, ref);
			return true;
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
			return std::begin(game->get_extras().objects);
		}

		auto objects_end()
		{
			return std::end(game->get_extras().objects);
		}

		std::vector<curve_t> get_all_curves(object_t o)
		{
			const auto& curves = resources::object_functions::get_all_curves(*o->object_type);
			auto out = std::vector<curve_t>{};
			out.reserve(size(curves));
			std::ranges::transform(curves, std::back_inserter(out), &resources::object::curve_obj::curve_ptr);
			return out;
		}

		const resources::curve* get_ptr(curve_t c) const noexcept
		{
			return c;
		}

		bool is_valid(curve_t c, const resources::curve_default_value& v) const noexcept
		{
			return resources::is_curve_valid(*c, v);
		}

		string get_name(curve_t c)
		{
			return to_string(c->id);
		}

		resources::curve_default_value copy_value(object_t o, curve_t c)
		{
			auto ret = resources::curve_default_value{};
			auto make_value = [&]<template<typename> typename CurveType, curve_type Type>() {
				const auto& prop = state_api::get_object_property_ref<CurveType, Type>(*o, c->id);
				// NOTE: we cannot edit pulse or const curves with this ui
				if constexpr (std::is_same_v<CurveType<Type>, step_curve<Type>> ||
					std::is_same_v<CurveType<Type>, linear_curve<Type>>)
				{
					const auto& val = prop.get(time);
					ret = val;
				}
				return;
			};

			state_api::detail::call_with_curve_info({ c->frame_style, c->data_type }, make_value);
			return ret;
		}

		void set_value(object_t o, curve_t c, curve_type auto v)
		{
			using Type = std::decay_t<decltype(v)>;
			auto set_value = [&]<template<typename> typename CurveType>() {
				auto& prop = state_api::get_object_property_ref<CurveType, Type>(*o, c->id);
				// NOTE: we cannot edit pulse or const curves with this ui
				if constexpr (std::is_same_v<CurveType<Type>, step_curve<Type>> ||
					std::is_same_v<CurveType<Type>, linear_curve<Type>>)
				{
					prop.replace_keyframes(time, std::move(v));
				}
				return;
			};

			state_api::detail::call_with_keyframe_style<Type>(c->frame_style, set_value);
			return;
		}

		void set_time(time_point t)
		{
			time = t;
			return;
		}
		
		game_interface* game = {};
		time_point time = time_point::min();

		static constexpr bool visual_editor = true;
		static constexpr bool keyframe_editor = false; // TODO: not implemented
		static constexpr bool edit_pulse_curves = keyframe_editor;
		static constexpr object_ref_t nothing_selected = hades::curve_types::bad_object_ref;
	};

	template<typename GameSystem>
	class object_inspector : public hades::debug::basic_overlay
	{
	public:
		object_inspector(game_interface* game)
			: _data{ game }, _editor_ui{ &_data }
		{
			assert(game);
			return;
		}

		void set_selected(hades::object_ref o) noexcept
		{
			_editor_ui.set_selected(o);
			return;
		}

		void set_time(time_point t) noexcept
		{
			_data.set_time(t);
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

		bool closed() const noexcept
		{
			return !_open;
		}

		void update(gui& g) override
		{
			using namespace std::string_view_literals;

			if (g.window_begin("object inspector"sv, _open))
			{
				if (g.collapsing_header("properties"sv, gui::tree_node_flags::default_open))
				{
					_editor_ui.object_properties(g);
				}
				
				if (g.collapsing_header("object list"sv, gui::tree_node_flags::default_open))
				{
					_editor_ui.show_object_list_buttons(g); 
					if (_editor_ui.object_list_gui(g))
						_sticky = false;
				}
			}
			g.window_end();

			return;
		}

	private:
		object_inspector_data<GameSystem> _data;
		object_editor<object_inspector_data<game_system>> _editor_ui;
		bool _sticky = false;
		bool _open = true;
	};
}

#endif // !HADES_OBJECT_INSPECTOR_HPP
