#ifndef HADES_OBJECT_EDITOR_HPP
#define HADES_OBJECT_EDITOR_HPP

#include <any>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "hades/collision_grid.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/level_editor_grid.hpp"
#include "hades/objects.hpp"
#include "hades/properties.hpp"
#include "hades/resource_base.hpp"

namespace hades
{
	namespace obj_ui
	{
		struct curve_cache_entry
		{
			string edit_buffer;
			std::size_t edit_generation = {};
			std::any extra_data;
		};

		using curve_edit_cache = unordered_map_string<curve_cache_entry>;

		// returns true if value was modified
		template<curve_type CurveType>
		bool edit_curve_value(gui&, std::string_view, curve_edit_cache&, bool disabled, CurveType& value);
		// collection specialisation
		template<curve_type CurveType>
		bool edit_curve_value(gui&, std::string_view, curve_edit_cache&, bool disabled, CurveType& value)
			requires curve_types::is_collection_type_v<CurveType>;

		// Base type, used for object_instance and editor_object_instance
		// see: object inspector for editing live objects
		template<typename ObjectType>
		struct object_data
		{
			using object_t = ObjectType*;
			using object_ref_t = entity_id;
			using curve_t = resources::object::curve_obj;
			
			const resources::object* get_type(object_t o)
			{
				return o->obj_type;
			}

			bool valid_ref(object_ref_t) const noexcept;
			object_t get_object(object_ref_t) noexcept;
			const object_t get_object(object_ref_t) const noexcept;
			object_ref_t get_ref(const object_t) const noexcept;

			object_ref_t add(object_instance);
			void remove(object_ref_t) noexcept;

			int32 to_int(const object_ref_t o) const noexcept
			{
				return integer_cast<int32>(static_cast<entity_id::value_type>(o));
			}

			string get_name(const object_t o) const
			{
				return o->name_id;
			}

			// returns true if the name could be set
			// doesn't take the name from another object
			bool set_name(object_t o, std::string_view s);

			string get_type_name(const object_t o) const
			{
				return o->obj_type ? data::get_as_string(o->obj_type->id) : to_string(o->id);
			}

			string get_id_string(const object_t o) const
			{
				return to_string(o->id);
			}

			bool has_curve(const object_t o, const resources::curve* c) const noexcept
			{
				// call the function from objects.hpp
				return hades::has_curve(*o, *c);
			}

			auto objects_begin()
			{
				return std::begin(objects);
			}
			
			auto objects_end()
			{
				return std::end(objects);
			}

			std::vector<curve_t> get_all_curves(object_t o)
			{
				return hades::get_all_curves(*o);
			}

			const resources::curve* get_ptr(curve_t c) const noexcept
			{
				return c.curve_ptr;
			}

			bool is_valid(curve_t c, const curve_value& v) const noexcept
			{
				return resources::is_curve_valid(*(c.curve_ptr), v);
			}

			string get_name(curve_t c)
			{
				return to_string(c.curve_ptr->id);
			}

			curve_value copy_value(object_t o, curve_t c)
			{
				return get_curve(*o, *c.curve_ptr);
			}

			void set_value(object_t o, curve_t c, curve_type auto v)
			{
				set_curve(*o, *(c.curve_ptr), std::move(v));
				return;
			}

			std::vector<ObjectType> objects;
			unordered_map_string<entity_id> entity_names;
			entity_id next_id = next(bad_entity);

			// if true, hide visible objects from the object list
			static constexpr bool visual_editor = !std::is_same_v<ObjectType, object_instance>;
			static constexpr bool keyframe_editor = false; // TODO: not implemented
			static constexpr bool edit_pulse_curves = true;
			static constexpr object_ref_t nothing_selected = bad_entity;
		};
	}

	template<typename ObjectData, typename OnChange = nullptr_t, typename OnRemove = nullptr_t>
		class object_editor
	{
	public:
		using data_type = ObjectData;
		using object_t = typename data_type::object_t;
		using object_ref_t = typename data_type::object_ref_t;
		using curve_t = typename data_type::curve_t;

		static constexpr bool on_change_callback = std::is_invocable_v<OnChange, object_t&>;
		static_assert(std::is_same_v<OnChange, nullptr_t> || on_change_callback,
			"If the OnChange callback is provided it must have the following signiture: auto OnChange(object_t&)");

		static constexpr bool on_remove_callback = std::is_invocable_v<OnRemove, object_ref_t>;
		static_assert(std::is_same_v<OnRemove, nullptr_t> || on_remove_callback,
			"If the OnRemove callback is provided it must have the following signiture: auto OnRemove(object_ref_t)");

		static constexpr bool can_add_objects = requires (ObjectData& data, object_instance o) { data.add(o); };
		static constexpr bool visual_editor = data_type::visual_editor;

		//constructor
		object_editor(data_type*, OnChange = nullptr, OnRemove = nullptr);

		void show_object_list_buttons(gui&);
		// returns true if something was selected through the ui
		bool object_list_gui(gui&);
		void object_properties(gui&);

		void set_selected(object_ref_t obj) noexcept;

		object_ref_t selected() const noexcept
		{
			return _selected;
		}

		object_t get_obj(object_ref_t ref) noexcept
		{
			return _data->get_object(ref);
		}

		const object_t get_obj(object_ref_t ref) const noexcept
		{
			return _data->get_object(ref);
		}

		object_ref_t add(object_instance o) requires can_add_objects;

		void erase(object_ref_t);

	private:
		struct curve_entry
		{
			data_type::curve_t curve = {};
			string name;
			curve_value value;
		};

		void _edit_name(gui&, object_t);

		string _get_name(const object_t o) const
		{
			const auto name = _data->get_name(o);
			const auto type = _data->get_type_name(o);

			using namespace std::string_literals;
			if (!name.empty())
				return name + "("s + type + ")"s;
			else
				return type;
		}

		string _get_name_with_tag(const object_t o) const
		{
			const auto name = _data->get_name(o);
			const auto type = _data->get_type_name(o);
			const auto id = _data->get_id_string(o);

			using namespace std::string_literals;
			if (!name.empty())
				return name + "("s + type + ")##"s + id;
			else
				return type + "##"s + id;
		}

		void _property_editor(gui&);
		
		// Edit state
		string _entity_name_id_cache;
		string _entity_name_id_uncommited;
		obj_ui::curve_edit_cache _edit_cache;

		// selection index for the available object bases when creating new objects
		std::size_t _next_added_object_base = std::size_t{};

		//callbacks
		OnChange _on_change{};
		OnRemove _on_remove{};

		//shared state
		data_type* _data = {};
		std::vector<curve_entry> _curves;
		object_ref_t _selected = data_type::nothing_selected;

		// hidden curves are not shown by default
		bool _show_hidden = false;
	};
}

#include "hades/detail/object_editor.inl"

#endif //!HADES_OBJECT_EDITOR_HPP
