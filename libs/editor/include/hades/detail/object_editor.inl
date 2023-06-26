#include "hades/object_editor.hpp"

#include "hades/core_curves.hpp"
#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/utility.hpp"

namespace hades::obj_ui
{
	template<curve_type CurveType, typename BinaryFunction>
	inline bool edit_curve_value(gui& g, std::string_view name, curve_edit_cache&,
		bool disabled, CurveType& value, BinaryFunction)
	{
		auto ret = false;
		if (disabled)
			g.begin_disabled();
		ret = g.input(name, value);
		if (disabled)
			g.end_disabled();
		return ret;
	}

	template<typename Callback = nullptr_t>
	inline bool edit_curve_value(gui& g, std::string_view name,	curve_edit_cache&,
		bool disabled, curve_types::vec2_float& value, Callback = {})
	{
		auto ret = false;
		if (disabled)
			g.begin_disabled();
		auto val2 = std::array{ value.x, value.y };
		if (g.input(name, val2))
		{
			value = { val2[0], val2[1] };
			ret = true;
		}
		if (disabled)
			g.end_disabled();
		return ret;
	}

	template<typename Callback = nullptr_t>	
	inline bool edit_curve_value(gui& g, std::string_view name,
		curve_edit_cache&, bool disabled, curve_types::object_ref& value, Callback cb = {})
	{
		auto ret = false;
		auto value2 = integer_cast<int>(to_value(value.id));

		if constexpr (std::is_invocable_v<Callback, gui&, const curve_types::object_ref&>)
			std::invoke(cb, g, value);

		if (disabled)
			g.begin_disabled();
		if (g.input(name, value2))
		{
			value = object_ref{ entity_id{ integer_cast<entity_id::value_type>(value2) } };
			ret = true;
		}
		if (disabled)
			g.end_disabled();
		return ret;
	}

	template<typename Callback = nullptr_t>
	inline bool edit_curve_value(gui& g, std::string_view name,
		curve_edit_cache&, bool disabled, curve_types::colour& value, Callback = {})
	{
		auto ret = false;
		auto arr = std::array{
			integer_cast<int>(value[0]),
				integer_cast<int>(value[1]),
				integer_cast<int>(value[2]),
				integer_cast<int>(value[3])
		};

		if (disabled)
			g.begin_disabled();
		if (g.input(name, arr))
		{
			value = {
				integer_clamp_cast<uint8>(arr[0]),
				integer_clamp_cast<uint8>(arr[1]),
				integer_clamp_cast<uint8>(arr[2]),
				integer_clamp_cast<uint8>(arr[3])
			};
			ret = true;
		}
		if (disabled)
			g.end_disabled();

		return ret;
	}

	template<typename Callback = nullptr_t>
	inline bool edit_curve_value(gui& g, std::string_view name,
		curve_edit_cache&, bool disabled, curve_types::bool_t& value, Callback = {})
	{
		auto ret = false;
		if (disabled)
			g.begin_disabled();
		ret = g.checkbox(name, value);
		if (disabled)
			g.end_disabled();
		return ret;
	}

	template<typename Callback = nullptr_t>
	inline bool edit_curve_value(gui& g, std::string_view name,
		curve_edit_cache&, bool disabled, curve_types::unique& value, Callback cb = {})
	{
		auto ret = false;
		// intentional copy
		string u_string = data::get_as_string(value);

		if constexpr (std::is_invocable_v<Callback, gui&, const curve_types::unique&>)
		{
			std::invoke(cb, g, value);
		}

		if (disabled)
			g.begin_disabled();
		if (g.input_text(name, u_string))
		{
			value = data::make_uid(u_string);
			ret = true;
		}
		if (disabled)
			g.end_disabled();
		return ret;
	}

	template<typename Callback = nullptr_t>
	inline bool edit_curve_value(gui& g, std::string_view name,
		curve_edit_cache& cache, bool disabled, curve_types::time_d& value, Callback = {})
	{
		using duration_extra_data = duration_ratio;

		using namespace std::string_view_literals;
		auto ret = false;

		auto iter = cache.find(name);
		auto& cache_entry = iter == end(cache) ? cache[to_string(name)] : iter->second;
		if (!cache_entry.extra_data.has_value())
			cache_entry.extra_data.emplace<duration_extra_data>();

		auto& ratio = std::any_cast<duration_extra_data&>(cache_entry.extra_data);


		if (cache_entry.edit_generation == 0)
		{
			std::tie(cache_entry.edit_buffer, ratio) = duration_to_string(value);
			++cache_entry.edit_generation;
		}

		const auto preview = [ratio]() noexcept {
			if (ratio == duration_ratio::seconds)
				return "seconds"sv;
			if (ratio == duration_ratio::millis)
				return "milliseconds"sv;
			if (ratio == duration_ratio::micros)
				return "microseconds"sv;
			return "nanoseconds"sv;
		}();

		if (disabled)
			g.begin_disabled();
		if (g.combo_begin("##duration_ratio"sv, preview))
		{
			if (g.selectable("seconds"sv, ratio == duration_ratio::seconds))
			{
				++cache_entry.edit_generation;
				const auto secs = time_cast<seconds>(value);
				cache_entry.edit_buffer = to_string(secs.count());
				ratio = duration_ratio::seconds;
				ret = true;
			}

			if (g.selectable("milliseconds"sv, ratio == duration_ratio::millis))
			{
				++cache_entry.edit_generation;
				const auto millis = time_cast<milliseconds>(value);
				cache_entry.edit_buffer = to_string(millis.count());
				ratio = duration_ratio::millis;
				ret = true;
			}

			if (g.selectable("microseconds"sv, ratio == duration_ratio::micros))
			{
				++cache_entry.edit_generation;
				const auto micros = time_cast<microseconds>(value);
				cache_entry.edit_buffer = to_string(micros.count());
				ratio = duration_ratio::micros;
				ret = true;
			}

			if (g.selectable("nanoseconds"sv, ratio == duration_ratio::nanos))
			{
				++cache_entry.edit_generation;
				const auto nanos = time_cast<nanoseconds>(value);
				cache_entry.edit_buffer = to_string(nanos.count());
				ratio = duration_ratio::nanos;
				ret = true;
			}

			g.combo_end();
		}

		g.push_id(integer_cast<int32>(cache_entry.edit_generation));
		if (g.input_text(name, cache_entry.edit_buffer, gui::input_text_flags::chars_decimal))
		{
			switch (ratio)
			{
			case duration_ratio::seconds:
			{
				const auto secs = seconds{ from_string<int64>(cache_entry.edit_buffer) };
				value = time_cast<time_duration>(secs);
			}break;
			case duration_ratio::millis:
			{
				const auto millis = milliseconds{ from_string<int64>(cache_entry.edit_buffer) };
				value = time_cast<time_duration>(millis);
			}break;
			case duration_ratio::micros:
			{
				const auto micros = microseconds{ from_string<int64>(cache_entry.edit_buffer) };
				value = time_cast<time_duration>(micros);
			}break;
			case duration_ratio::nanos:
			{
				const auto nanos = nanoseconds{ from_string<int64>(cache_entry.edit_buffer) };
				value = time_cast<time_duration>(nanos);
			}break;
			default:
				throw out_of_range_error{"out of range"};
			}

			ret = true;
		}
		if (disabled)
			g.end_disabled();
		g.pop_id();
		return ret;
	}

	template<curve_type CurveType, typename Callback = nullptr_t>
		requires curve_types::is_collection_type_v<CurveType>
	inline bool edit_curve_value(gui& g, std::string_view name, curve_edit_cache& cache,
		bool disabled, CurveType& value, Callback cb = {})
	{
		using namespace std::string_view_literals;

		using vector_window_open = bool;
		// TODO: move this up into property_edit
		//		so this function can be used more generically by the resource editor
		// create vector window
		g.push_id(name);
		auto iter = cache.find(name);
		if (g.button("view"sv) && iter == end(cache))
			std::tie(iter, std::ignore) = cache.emplace(to_string(name), obj_ui::curve_cache_entry{});
		g.pop_id();

		enum class elem_mod {
			remove,
			move_up,
			move_down,
			error
		};

		auto changed = false;

		if (iter != end(cache))
		{
			auto open = true;
			if (g.window_begin(name, open))
			{
				constexpr auto max_index = std::numeric_limits<std::size_t>::max();
				auto index = max_index;
				auto mod = elem_mod::error;
				const auto size = std::size(value);
				const auto max = size - 1;

				g.text("elements:"sv);

				if (disabled)
					g.begin_disabled();

				if (g.button("add new"sv))
				{
					value.emplace_back(typename CurveType::value_type{});
					changed = true;
				}

				if (disabled)
					g.end_disabled();

				for (auto i = std::size_t{}; i != size; ++i)
				{
					g.push_id(integer_cast<int32>(i));
					g.separator_horizontal();

					if (i == 0)
						g.begin_disabled();
					if (g.button("^"sv))
					{
						index = i;
						mod = elem_mod::move_up;
					}
					if (i == 0)
						g.end_disabled();
					g.same_line();

					if (i == max)
						g.begin_disabled();
					if (g.button("v"sv))
					{
						index = i;
						mod = elem_mod::move_down;
					}
					if (i == max)
						g.end_disabled();
					g.same_line();

					if (disabled)
						g.begin_disabled();

					if (g.button("remove"sv))
					{
						index = i;
						mod = elem_mod::remove;
					}

					if (disabled)
						g.end_disabled();
					
					if (edit_curve_value(g, {}, cache, disabled, value[i], cb))
						changed = true;

					g.pop_id();
				}

				if (index != max_index &&
					mod != elem_mod::error)
				{
					switch (mod)
					{
					case elem_mod::move_up:
						std::swap(value[index], value[index - 1]);
						break;
					case elem_mod::move_down:
						std::swap(value[index], value[index + 1]);
						break;
					case elem_mod::remove:
						value.erase(next(begin(value), index));
					}
					changed = true;
				}

				g.layout_horizontal();
			}
			g.window_end();

			if (!open)
				cache.erase(iter);
		}

		g.same_line();
		if (disabled)
			g.begin_disabled();
		g.text(name);
		if (disabled)
			g.end_disabled();
		return changed;
	}

	template<typename ObjectType>
	inline bool object_data<ObjectType>::valid_ref(object_ref_t ref) const noexcept
	{
		return std::ranges::find(objects, ref, &ObjectType::id) != end(objects);
	}

	template<typename ObjectType>
	inline auto object_data<ObjectType>::get_object(object_ref_t ref) noexcept -> object_t
	{
		const auto iter = std::ranges::find(objects, ref, &ObjectType::id);
		return iter != end(objects) ? &*iter : nullptr;
	}

	template<typename ObjectType>
	inline auto object_data<ObjectType>::get_object(object_ref_t ref) const noexcept -> const object_t
	{
		const auto iter = std::ranges::find(objects, ref, &ObjectType::id);
		return iter != end(objects) ? &*iter : nullptr;
	}

	template<typename ObjectType>
	inline auto object_data<ObjectType>::get_ref(const object_t o) const noexcept -> object_ref_t
	{
		return o->id;
	}

	template<typename ObjectType>
	inline auto object_data<ObjectType>::add(object_instance o) -> object_ref_t
	{
		assert(o.id == bad_entity);
		const auto id = o.id = post_increment(next_id);
		objects.emplace_back(std::move(o));
		return id;
	}

	template<typename ObjectType>
	inline void object_data<ObjectType>::remove(object_ref_t ref) noexcept
	{
		const auto iter = std::ranges::find(objects, ref, &ObjectType::id);
		if (iter == end(objects))
		{
			assert(false);
			return;
		}

		entity_names.erase(iter->name_id);
		objects.erase(iter);
		return;
	}

	template<typename ObjectType>
	inline bool object_data<ObjectType>::set_name(object_t o, std::string_view s)
	{
		if (s.empty()) // remove name
		{
			//remove name_id
			auto begin = std::cbegin(entity_names);
			for (/*begin*/; ; ++begin)
			{
				assert(begin != std::cend(entity_names));
				if (begin->second == o->id)
					break;
			}

			entity_names.erase(begin);
			o->name_id.clear();
			return true;
		}
		
		// Name is already taken
		auto iter = entity_names.find(s);
		if (iter != end(entity_names))
			return false;

		// Set or replace current name
		// remove current name binding if present
		if (!o->name_id.empty())
		{
			for (auto begin = std::cbegin(entity_names); ; ++begin)
			{
				assert(begin != std::cend(entity_names));
				if (begin->second == o->id)
				{
					entity_names.erase(begin);
					break;
				}
			}
		}

		//apply
		entity_names.emplace(s, o->id);
		o->name_id = s;

		return true;
	}
}

namespace hades
{
	template<typename ObjectData, typename OnChange, typename OnRemove, typename CurveGuiCallback>
	object_editor<ObjectData, OnChange, OnRemove, CurveGuiCallback>::object_editor(ObjectData* d,
		OnChange on_change, OnRemove on_remove, CurveGuiCallback curve_gui_callback)
		: _data{ d }, _on_change{ on_change }, _on_remove{ on_remove }, _curve_edit_callback{ curve_gui_callback }
	{}

	template<typename ObjectData, typename OnChange, typename OnRemove, typename CurveGuiCallback>
	inline void object_editor<ObjectData, OnChange, OnRemove, CurveGuiCallback>::show_object_list_buttons(gui& g)
	{
		using namespace std::string_view_literals;
		using namespace std::string_literals;
		const auto& objs = resources::all_objects;

		assert(_next_added_object_base < std::size(objs) + 1 || std::empty(objs));

		const auto preview = [&objs](std::size_t index) {
			if (index == std::size_t{})
				return "none"s;
			else
				return to_string(objs[index - 1]->id);
		}(_next_added_object_base);

		const auto position_curve = get_position_curve();
		const auto size_curve = get_size_curve();

		if (g.combo_begin("object type"sv, preview))
		{
			if (g.selectable("none"sv, _next_added_object_base == std::size_t{}))
				_next_added_object_base = std::size_t{};

			const auto end = std::size(objs);
			for (auto i = std::size_t{}; i < end; ++i)
			{
				const auto name = to_string(objs[i]->id);

				if constexpr (visual_editor)
				{
					using resources::object_functions::has_curve;
					if (has_curve(*objs[i], *position_curve) || has_curve(*objs[i], *size_curve))
						continue;
				}

				if (g.selectable(name, _next_added_object_base == i + 1))
					_next_added_object_base = i + 1;
			}

			g.combo_end();
		}

		if (_next_added_object_base == std::size_t{})
			g.begin_disabled();

		if constexpr (can_add_objects)
		{
			if (g.button("add"sv))
			{
				const auto& o_type = objs[_next_added_object_base - 1];
				// NOTE: need to force load the object type
				data::get<resources::object>(o_type.id());
				add(make_instance(o_type.get()));
			}
		}
		else
		{
			g.begin_disabled();
			g.button("add"sv);
			g.end_disabled();
		}

		if (_next_added_object_base == std::size_t{})
			g.end_disabled();

		g.layout_horizontal();
		if (g.button("remove"sv) && _selected != data_type::nothing_selected)
			erase(_selected);
		return;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove, typename CurveGuiCallback>
	inline bool object_editor<ObjectData, OnChange, OnRemove, CurveGuiCallback>::object_list_gui(gui& g)
	{
		using namespace std::string_view_literals;
		
		// only list the invisible objects for the visual editor
		const auto position_curve = get_position_curve();
		const auto size_curve = get_size_curve();

		auto sel = false;
		if (g.listbox_begin("##obj_list"sv))
		{
			auto iter = _data->objects_begin();
			const auto end = _data->objects_end();
			for (iter; iter != end; ++iter)
			{
				auto o = &*iter;
				auto ref = _data->get_ref(o);

				if constexpr (visual_editor)
				{
					if (!_data->get_name(o).empty() ||
						!(_data->has_curve(o, position_curve) && _data->has_curve(o, size_curve)))
					{
						if (g.selectable(_get_name_with_tag(o), ref != data_type::nothing_selected && ref == selected()))
						{
							set_selected(ref);
							sel = true;
						}
					}
				}
				else
				{
					if (g.selectable(_get_name_with_tag(o), ref != data_type::nothing_selected && ref == selected()))
					{
						set_selected(ref);
						sel = true;
					}
				}
			}
			g.listbox_end();
		}

		return sel;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove, typename CurveGuiCallback>
	inline void object_editor<ObjectData, OnChange, OnRemove, CurveGuiCallback>::object_properties(gui& g)
	{
		using namespace std::string_view_literals;
		if (!_data->valid_ref(_selected))
			_selected = data_type::nothing_selected;

		if (_selected == data_type::nothing_selected)
		{
			//_vector_curve_edit = {};
			g.text("Nothing is selected..."sv);
			return;
		}

		g.push_id(_data->to_int(_selected));
		_property_editor(g);
		g.pop_id();
		return;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove, typename CurveGuiCallback>
	inline void object_editor<ObjectData, OnChange, OnRemove, CurveGuiCallback>::set_selected(object_ref_t ref) noexcept
	{
		if (!_data->valid_ref(ref))
		{
			_selected = data_type::nothing_selected;
			return;
		}

		const auto o = _data->get_object(ref);
		_selected = ref;

		// stash the objects curve list
		auto all_curves = _data->get_all_curves(o);
		_curves.clear();

		for (auto& c : all_curves)
		{
			const resources::curve* c_ptr = _data->get_ptr(c);
			if (c_ptr->frame_style == keyframe_style::const_t)
			{
				const auto obj_type = _data->get_type(o);
				auto val = resources::object_functions::get_curve(*obj_type, *c_ptr);
				_curves.emplace_back(c, _data->get_name(c), std::move(val));
			}
			else
			{
				auto val = _data->copy_value(o, c);
				_curves.emplace_back(c, _data->get_name(c), std::move(val));
			}
		}

		std::ranges::sort(_curves, {}, &curve_entry::name);
		_entity_name_id_cache = _entity_name_id_uncommited = _data->get_name(o);
		const auto type = _data->get_type(o);
		_obj_type_str = data::get_as_string(type->id);
		return;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove, typename CurveGuiCallback>
	inline auto object_editor<ObjectData, OnChange, OnRemove, CurveGuiCallback>::add(object_instance o) -> object_ref_t
		requires can_add_objects
	{
		const auto new_obj = _data->add(std::move(o));
		set_selected(new_obj);
		return new_obj;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove, typename CurveGuiCallback>
	inline void object_editor<ObjectData, OnChange, OnRemove, CurveGuiCallback>::erase(object_ref_t ref)
	{
		if constexpr (on_remove_callback)
			std::invoke(_on_remove, ref);
		_data->remove(ref);
		set_selected(data_type::nothing_selected);
		return;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove, typename CurveGuiCallback>
	inline void object_editor<ObjectData, OnChange, OnRemove, CurveGuiCallback>::_edit_name(gui& g, const object_t o)
	{
		using namespace std::string_view_literals;

		enum class reason {
			ok,
			already_taken,
			reserved_name
		};

		auto name_reason = reason::ok;
		auto& text = _entity_name_id_uncommited;
		const auto old_name = _data->get_name(o);

		if (g.input_text("Name_id"sv, text))
		{
			//if the new name is empty, and the old name isn't
			if (text == string{}
				&& !old_name.empty())
			{
				_data->set_name(o, {});
			}
			else if(!_data->set_name(o, text))
				name_reason = reason::already_taken;
		}

		if (name_reason == reason::already_taken)
			g.tooltip("This name is already being used"sv);
		return;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove, typename CurveGuiCallback>
	inline void object_editor<ObjectData, OnChange, OnRemove, CurveGuiCallback>::_property_editor(gui& g)
	{
		using namespace std::string_view_literals;
		using namespace std::string_literals;

		const auto o = _data->get_object(_selected);
		if (!o)
		{
			set_selected(data_type::nothing_selected);
			return;
		}

		const auto name = _data->get_name(o);

		g.text("Selected: "s + name);
		g.push_id(_data->to_int(_selected));

		//properties
		//immutable object id
		auto id_str = to_string(o->id);
		g.begin_disabled();
		g.input_text("id"sv, id_str);
		g.end_disabled();
		// object name
		_edit_name(g, o);
		// object_type
		auto type = _data->get_type(o);
		auto type_id = type->id;
		obj_ui::edit_curve_value(g, "object type"sv, _edit_cache, true, type_id, _curve_edit_callback);

		g.text("curves:"sv);
		g.separator_horizontal();

		// all other properties
		for (auto& c : _curves)
		{
			const resources::curve* c_ptr = _data->get_ptr(c.curve);

			if (c_ptr->frame_style != keyframe_style::const_t)
			{
				auto val = _data->copy_value(o, c.curve);
				if (val != c.value)
				{
					c.value = std::move(val);
					auto iter = _edit_cache.find(c.name);
					if (iter != end(_edit_cache))
					{
						iter->second.edit_generation = {};
					}
				}
			}

			// TODO: test data_type::keyframe_editor and generate a different UI
			if (!_data->is_valid(c.curve, c.value))
				continue;

			g.push_id(c_ptr);
			std::visit([&](auto&& value) {
				using Type = std::decay_t<decltype(value)>;
				
				if constexpr (!show_hidden)
				{
					// dont show hidden props
					if (c_ptr->hidden)
						return;
				}

				if constexpr (!std::is_same_v<std::monostate, Type>)
				{
					auto disabled = false;
					assert(resources::is_curve_valid(*c_ptr, c.value));
					if (c_ptr->frame_style == keyframe_style::const_t || // dont edit const
						// only edit pulse curves if enabled
						(c_ptr->frame_style == keyframe_style::pulse && !data_type::edit_pulse_curves) ||
						// curve is locked(not writable in editor)
						c_ptr->locked)
						disabled = true;

					if (obj_ui::edit_curve_value(g, c.name, _edit_cache, disabled, value, _curve_edit_callback))
					{
						_data->set_value(o, c.curve, value);

						if constexpr (on_change_callback)
							std::invoke(_on_change, o);
					}

					g.separator_horizontal();
				}
			}, c.value);
			g.pop_id(); // curve address
		}

		g.pop_id(); // entity_id

		if constexpr (default_curve_callback)
		{
			const auto select_ref = _curve_edit_callback.get_reset_target();
			if(select_ref != curve_types::bad_object_ref)
				set_selected(select_ref);
		}

		return;
	}
}
