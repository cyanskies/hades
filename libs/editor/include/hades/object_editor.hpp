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
		// TODO: merge these edit caches into a generic cache
		//		needed to cache unique_id pickers, colour pickers
		//		keyframe based editor
		struct vector_edit_window
		{
			static constexpr auto nothing_selected = std::numeric_limits<std::size_t>::max();
			std::size_t selected = nothing_selected;
		};

		using vector_window_map = unordered_map_string<vector_edit_window>;

		struct duration_edit_cache
		{
			string edit_buffer;
			int32 edit_generation = 0;
			duration_ratio ratio;
		};

		using duration_cache_map = unordered_map_string<duration_edit_cache>;

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
		template<curve_type CurveType>
		bool _property_row(gui&, std::string_view, curve_t, bool disabled, CurveType& value);
		// specialisations
		bool _property_row(gui&, std::string_view, curve_t, bool disabled, curve_types::vec2_float& value);
		bool _property_row(gui&, std::string_view, curve_t, bool disabled, curve_types::object_ref& value);
		bool _property_row(gui&, std::string_view, curve_t, bool disabled, curve_types::colour& value);
		bool _property_row(gui&, std::string_view, curve_t, bool disabled, curve_types::bool_t& value);
		bool _property_row(gui&, std::string_view, curve_t, bool disabled, curve_types::unique& value);
		bool _property_row(gui&, std::string_view, curve_t, bool disabled, curve_types::time_d& value);
		// collection specialisation
		template<curve_type CurveType>
		bool _property_row(gui&, std::string_view, curve_t, bool disabled, CurveType& value)
			requires curve_types::is_collection_type_v<CurveType>;

		// Edit state
		string _entity_name_id_cache;
		string _entity_name_id_uncommited;
		duration_cache_map _duration_edit_cache;
		vector_window_map _vector_edit_windows;

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


	// UI for editing objects
	// Needs to support:
	//		object_instance
	//		editor_object_instance
	// // TODO:
	//		live game objects
	//		editor objects with curves
	template<typename ObjectType = object_instance, typename OnChange = nullptr_t, typename IsValidPos = nullptr_t>
	class object_editor_ui_old
	{
	public:
		static_assert(std::is_base_of_v<object_instance, ObjectType> ||
			std::is_same_v<object_instance, ObjectType> ||
			std::is_same_v<nullptr_t, ObjectType>); // nullptr is used only to access subclassess without using the full editor

		static_assert(std::is_same_v<OnChange, nullptr_t> ||
			std::is_invocable_v<OnChange, ObjectType&>,
			"If the OnChange callback is provided it must have the following signiture: auto OnChange(ObjectType&)");

		static_assert(std::is_same_v<IsValidPos, nullptr_t> ||
			std::is_invocable_r_v<bool, IsValidPos, const rect_float&, const object_instance&>,
			"If the IsValidPos callback is provided it must have the following signiture: bool IsValidPos(const rect_float&, const object_instance&)");

		static_assert(!std::is_same_v<OnChange, IsValidPos> || 
            (std::is_same_v<IsValidPos, nullptr_t> && std::is_same_v<OnChange, nullptr_t>),
			"If one of the editor callbacks are provided, then both must be.");

		static constexpr bool visual_editor = !std::is_same_v<OnChange, nullptr_t>;

		using object_type = ObjectType;

		struct object_data
		{
			std::vector<ObjectType> objects;
			unordered_map_string<entity_id> entity_names;
			entity_id next_id = next(bad_entity);
		};

		struct vector_curve_edit
		{
			std::size_t selected = 0;
			const resources::curve* target = nullptr;
		};

		explicit object_editor_ui_old(object_data* d) noexcept
			: _data{d}
		{
			assert(_data);
		}

		object_editor_ui_old(object_data* d, OnChange change_func, IsValidPos isvalid_func)
        : _on_change{ change_func }, _is_valid_pos{ isvalid_func }, _data{ d }
		{
			assert(_data);
		}

		void show_object_list_buttons(gui&);
		void object_list_gui(gui&);
		void object_properties(gui&);

		void set_selected(entity_id) noexcept;
		ObjectType* get_obj(entity_id) noexcept;
		const ObjectType* get_obj(entity_id) const noexcept;
		entity_id add(ObjectType);
		void erase(entity_id);

		struct curve_edit_cache
		{
			string edit_buffer;
			int32 edit_generation = 0;
			std::any extra_data;
		};

		using cache_map = unordered_map_string<curve_edit_cache>;

	private:
		struct curve_info
		{
			const resources::curve* curve = nullptr;
			resources::curve_default_value value;
		};

		enum class curve_index : std::size_t {
			pos,
			size_index
		};

		struct add_remove_curve_window
		{
			enum class window_state {
				closed,
				add,
				remove
			};

			window_state state = window_state::closed;
			std::size_t list_index = std::size_t{};
		};

		void _add_remove_curve_window(gui&);
		void _erase(std::size_t);
		std::size_t _get_obj(entity_id) const noexcept;
		ObjectType* _get_obj(std::size_t) noexcept;
		void _reset_add_remove_curve_window() noexcept;
		template<typename MakeRect>
		void _positional_property_field(gui&, std::string_view, ObjectType&, 
			curve_info&, MakeRect&&);
		void _property_editor(gui&);
		void _set_selected(std::size_t);

		//callbacks
		OnChange _on_change{};
		IsValidPos _is_valid_pos{};

		//shared state
		object_data *_data = nullptr;

		//editing and ui data
		std::size_t _obj_list_selected = std::size_t{};
		std::size_t _next_added_object_base = std::size_t{};
		entity_id _selected = bad_entity;
		curve_list _curves;
		add_remove_curve_window _add_remove_window_state;
		std::string _entity_name_id_uncommited;
		vector_curve_edit _vector_curve_edit;
		cache_map _edit_cache;
		std::array<curve_info, 2> _curve_properties;
	};

	template<typename DataType, typename OnChange = nullptr_t, typename OnRemove = nullptr_t>
	using object_editor_ui = object_editor<DataType, OnChange, OnRemove>;

	bool make_curve_default_value_editor(gui& g, std::string_view name,
		const resources::curve* c, resources::curve_default_value& value,
		typename object_editor_ui_old<nullptr_t>::vector_curve_edit& target,
		typename object_editor_ui_old<nullptr_t>::cache_map& cache);
}

#include "hades/detail/object_editor.inl"

#endif //!HADES_OBJECT_EDITOR_HPP
