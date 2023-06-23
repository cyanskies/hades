#include "hades/object_editor.hpp"

#include "hades/core_curves.hpp"
#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/exceptions.hpp"
#include "hades/gui.hpp"
#include "hades/utility.hpp"

namespace hades::detail::obj_ui
{
	//TODO: is this worth moving to the util lib
	template<std::size_t Length>
	[[deprecated]]
	string clamp_length(std::string_view str)
	{
		if (str.length() <= Length)
			return to_string(str);

		using namespace std::string_literals;
		return to_string(str.substr(0, Length)) + "..."s;
	}

	static inline void make_name_id_property(gui& g, object_instance& o, string& text, unordered_map_string<entity_id>& name_map)
	{
		using namespace std::string_view_literals;

		enum class reason {
			ok,
			already_taken,
			reserved_name
		};

		auto name_reason = reason::ok;

		if (g.input_text("Name_id"sv, text))
		{
			//if the new name is empty, and the old name isn't
			if (text == string{}
				&& !o.name_id.empty())
			{
				//remove name_id
				auto begin = std::cbegin(name_map);
                for (/*begin*/; ; ++begin)
				{
					assert(begin != std::cend(name_map));
					if (begin->second == o.id)
						break;
				}

				name_map.erase(begin);
				o.name_id.clear();
			}
			//TODO: handle reserved names
			/*else if (const auto iter = std::find(std::begin(reserved_object_names),
				std::end(reserved_object_names), text); iter != std::end(reserved_object_names))
			{
				name_reason = reason::reserved_name;
			}*/
			else if (const auto iter = name_map.find(text);
				iter == std::end(name_map))
			{
				//remove current name binding if present
				if (!o.name_id.empty())
				{
					for (auto begin = std::cbegin(name_map); ; ++begin)
					{
						assert(begin != std::cend(name_map));
						if (begin->second == o.id)
						{
							name_map.erase(begin);
							break;
						}
					}
				}

				//apply
				name_map.emplace(text, o.id);
				o.name_id = text;
			}
			else
				name_reason = reason::already_taken;
		}

		//if the editbox is different to the current name,
		//then something must have stopped it from being commited
		if (name_reason == reason::already_taken)
		{
			if (const auto iter = name_map.find(text); iter->second != o.id)
				g.tooltip("This name is already being used"sv);
		}
		/*else if (name_reason == reason::reserved_name)
			g.show_tooltip("This name is reserved"sv);*/
		return;
	}

	template<typename T>
	struct has_custom_edit_func : std::false_type {};

	template<typename ObjEditor, typename T>
	std::optional<T> make_property_edit_basic(gui& g, std::string_view name, const T& value,
		typename ObjEditor::cache_map&)
	{
		auto ret = std::optional<T>{};
		auto edit_value = value;
		if (g.input(name, edit_value))
			ret = edit_value;
		g.tooltip(name);

		return ret;
	}

	// NOTE: unused
	template<>
	struct has_custom_edit_func<int64> : std::true_type {};

	template<typename ObjEditor>
	inline std::optional<int64> make_property_edit_custom(gui& g, std::string_view name, const int64& value,
		typename ObjEditor::cache_map&)
	{
		auto ret = std::optional<int64>{};
		auto value2 = integer_cast<int>(value);
		if (g.input(name, value2))
			ret = value2;
		g.tooltip(name);
		return ret;
	}

	template<>
	struct has_custom_edit_func<object_ref> : std::true_type {};

	template<typename ObjEditor>
	inline std::optional<object_ref> make_property_edit_custom(gui& g, std::string_view name, const object_ref& value,
		typename ObjEditor::cache_map&)
	{
		auto ret = std::optional<object_ref>{};
		auto value2 = integer_cast<int>(to_value(value.id));
		if (g.input(name, value2))
			ret = object_ref{ entity_id{integer_cast<entity_id::value_type>(value2)} };
		g.tooltip(name);
		return ret;
	}

	template<>
	struct has_custom_edit_func<colour> : std::true_type {};

	template<typename ObjEditor>
	inline std::optional<colour> make_property_edit_custom(gui& g, std::string_view name, const colour& value,
		typename ObjEditor::cache_map&)
	{
		auto ret = std::optional<colour>{};
		auto arr = std::array{
			integer_cast<int>(value[0]),
			integer_cast<int>(value[1]),
			integer_cast<int>(value[2]),
			integer_cast<int>(value[3])
		};

		if (g.input(name, arr))
		{
			ret = colour{
				integer_clamp_cast<uint8>(arr[0]),
				integer_clamp_cast<uint8>(arr[1]),
				integer_clamp_cast<uint8>(arr[2]),
				integer_clamp_cast<uint8>(arr[3])
			};
		}

		g.tooltip(name);
		return ret;
	}

	template<>
	struct has_custom_edit_func<bool> : std::true_type {};

	template<typename ObjEditor>
	inline std::optional<bool> make_property_edit_custom(gui& g, std::string_view name, const bool& value,
		typename ObjEditor::cache_map&)
	{
		using namespace std::string_view_literals;
		constexpr auto tru = "true"sv;
		constexpr auto fal = "false"sv;

		auto ret = std::optional<bool>{};
		if (g.combo_begin(name, value ? tru : fal))
		{
			g.tooltip(name);
			if (g.selectable(tru, value))
				ret = true;
			if (g.selectable(fal, !value))
				ret = false;

			g.combo_end();
		}

		return ret;
	}

	template<>
	struct has_custom_edit_func<string> : std::true_type {};

	template<typename ObjEditor>
	inline std::optional<string> make_property_edit_custom(gui& g, std::string_view name, const string& value,
		typename ObjEditor::cache_map&)
	{
		auto ret = std::optional<string>{};
		auto edit = value;
		if (g.input_text(name, edit))
			ret = std::move(edit);
		g.tooltip(name);
		return ret;
	}

	template<>
	struct has_custom_edit_func<unique_id> : std::true_type {};

	template<typename ObjEditor>
	inline std::optional<unique_id> make_property_edit_custom(gui& g, std::string_view name, const unique_id& value,
		typename ObjEditor::cache_map&)
	{
		auto ret = std::optional<unique_id>{};
		auto u_string = data::get_as_string(value);
		if (g.input_text(name, u_string))
			ret = data::make_uid(u_string);
		g.tooltip(name);
		return ret;
	}

	template<>
	struct has_custom_edit_func<time_duration> : std::true_type {};

	template<typename ObjEditor>
	inline std::optional<time_duration> make_property_edit_custom(gui& g, std::string_view name, const time_duration& value,
		typename ObjEditor::cache_map& cache)
	{
		using namespace std::string_view_literals;

		auto iter = cache.find(name);
		auto& cache_entry = iter == end(cache) ? cache[to_string(name)] : iter->second;
		if (cache_entry.edit_generation == 0)
		{
			std::tie(cache_entry.edit_buffer, cache_entry.extra_data) = duration_to_string(value);
			++cache_entry.edit_generation;
		}

		auto ratio_ptr = std::any_cast<duration_ratio>(&cache_entry.extra_data);
		assert(ratio_ptr);
		auto& ratio = *ratio_ptr;

		const auto preview = [ratio]() noexcept {
			if (ratio == duration_ratio::seconds)
				return "seconds"sv;
			if (ratio == duration_ratio::millis)
				return "milliseconds"sv;
			if (ratio == duration_ratio::micros)
				return "microseconds"sv;
			return "nanoseconds"sv;
		}();

		if (g.combo_begin("##duration_ratio"sv, preview))
		{
			if (g.selectable("seconds"sv, ratio == duration_ratio::seconds))
			{
				++cache_entry.edit_generation;
				const auto secs = time_cast<seconds>(value);
				cache_entry.edit_buffer = to_string(secs.count());
				ratio = duration_ratio::seconds;
			}

			if (g.selectable("milliseconds"sv, ratio == duration_ratio::millis))
			{
				++cache_entry.edit_generation;
				const auto millis = time_cast<milliseconds>(value);
				cache_entry.edit_buffer = to_string(millis.count());
				ratio = duration_ratio::millis;
			}

			if (g.selectable("microseconds"sv, ratio == duration_ratio::micros))
			{
				++cache_entry.edit_generation;
				const auto micros = time_cast<microseconds>(value);
				cache_entry.edit_buffer = to_string(micros.count());
				ratio = duration_ratio::micros;
			}

			if (g.selectable("nanoseconds"sv, ratio == duration_ratio::nanos))
			{
				++cache_entry.edit_generation;
				const auto nanos = time_cast<nanoseconds>(value);
				cache_entry.edit_buffer = to_string(nanos.count());
				ratio = duration_ratio::nanos;
			}

			g.combo_end();
		}

		auto out = std::optional<time_duration>{};
		
		g.push_id(cache_entry.edit_generation);
		if (g.input_text(name, cache_entry.edit_buffer, gui::input_text_flags::chars_decimal))
		{
			switch (ratio)
			{
			case duration_ratio::seconds:
			{
				const auto secs = seconds{ from_string<int64>(cache_entry.edit_buffer) };
				out = time_cast<time_duration>(secs);
			}break;
			case duration_ratio::millis:
			{
				const auto millis = milliseconds{ from_string<int64>(cache_entry.edit_buffer) };
				out = time_cast<time_duration>(millis);
			}break;
			case duration_ratio::micros:
			{
				const auto micros = microseconds{ from_string<int64>(cache_entry.edit_buffer) };
				out = time_cast<time_duration>(micros);
			}break;
			case duration_ratio::nanos:
			{
				const auto nanos = nanoseconds{ from_string<int64>(cache_entry.edit_buffer) };
				out = time_cast<time_duration>(nanos); 
			}break;
			default:
				throw out_of_range_error{"out of range"};
			}
		}

		g.tooltip(name);
		g.pop_id();
		return out;
	}

	template<typename ObjEditor, typename T>
	inline std::optional<T> make_property_edit_impl(gui& g, std::string_view name, const T& value,
		typename ObjEditor::cache_map& cache)
	{
		// TODO: do we need this seperation?
		// canot be just overload make_property_edit_basic?
		if constexpr (has_custom_edit_func<T>::value)
			return make_property_edit_custom<ObjEditor>(g, name, value, cache);
		else
			return make_property_edit_basic<ObjEditor>(g, name, value, cache);
	}

	template<typename ObjEditor, typename T>
	bool make_property_edit(gui& g, typename ObjEditor::object_type& o, std::string_view name, const resources::curve& c, T& value,
		typename ObjEditor::cache_map& cache)
	{
		constexpr auto use_object_type = !std::is_same_v<nullptr_t, typename ObjEditor::object_type>;
		const auto disabled = use_object_type && (c.locked || c.frame_style == keyframe_style::const_t);

		using namespace std::string_view_literals;

		if (disabled)
			g.begin_disabled();
		auto ret = false;
		if constexpr (resources::curve_types::is_vector_type_v<T>)
		{
			auto arr = std::array{ value.x, value.y };
			if (g.input(name, arr))
			{
				ret = true;
				if constexpr (use_object_type)
					set_curve(o, c, T{ arr[0], arr[1] });
			}

			value.x = arr[0];
			value.y = arr[1];

			g.tooltip(name);
		}
		else
		{
			auto new_value = make_property_edit_impl<ObjEditor>(g, name, value, cache);
			if (new_value)
			{
				ret = true;
				if constexpr (use_object_type)
				{
					value = *new_value;
					set_curve(o, c, std::move(*new_value));
				}
				else
					value = std::move(*new_value);
			}
		}

		if (disabled)
			g.end_disabled();

		return ret;
	}

	template<typename ObjEditor, typename T>
	bool make_vector_edit_field(gui& g, typename ObjEditor::object_type& o, const resources::curve& c, int32 selected, T& value,
		typename ObjEditor::cache_map& cache)
	{
		using namespace std::string_view_literals;
		auto iter = std::cbegin(value);
		std::advance(iter, selected);

		const typename T::value_type& value_ref = *iter;
		auto result = make_property_edit_impl<ObjEditor>(g, "edit"sv, value_ref, cache);
		if (result)
		{
			auto& container = value;
			auto target = std::begin(container);
			std::advance(target, selected);
			*target = std::move(*result);
			// for calling without an object type (don't call set_curve)
			if constexpr (!std::is_same_v<typename ObjEditor::object_type, nullptr_t>)
				set_curve(o, c, container);

			return true;
		}
		return false;
	}

	template<typename T, typename U, typename V, typename Obj>
	bool make_vector_property_edit(gui& g, Obj& o, std::string_view name,
		const resources::curve* c, T& value, typename object_editor_ui_old<Obj, U, V>::vector_curve_edit& target,
		typename object_editor_ui_old<Obj, U, V>::cache_map& cache)
	{
		using namespace std::string_view_literals;
		using vector_curve_edit = typename object_editor_ui_old<Obj, U, V>::vector_curve_edit;

		constexpr auto use_object_type = !std::is_same_v<Obj, nullptr_t>;

		if (g.button("edit vector..."sv))
			target.target = c;
		g.layout_horizontal();
		g.text(name);

		const auto disabled = use_object_type && (c->locked || c->frame_style == keyframe_style::const_t);

		// NOTE: throughout this func we don't std::move container
		// this is because we want to copy the updated value into the object
		// but also keep the changed object in our curve cache(the source
		// of &value)
		auto& container = value;

		auto ret = false;
		if (target.target == c)
		{
			if (g.window_begin("edit vector"sv, gui::window_flags::no_collapse))
			{
				if (g.button("done"sv))
					target = vector_curve_edit{};

				g.text(name);
				g.columns_begin(2u, false);

                constexpr auto& to_str = func_ref<string(typename T::value_type)>(to_string);
                g.listbox("##listbox", target.selected, value, to_str);

				g.columns_next();

				if (disabled)
					g.begin_disabled();

				assert(target.selected >= 0);
				if (static_cast<std::size_t>(target.selected) < std::size(value))
				{
					ret = make_vector_edit_field<object_editor_ui_old<Obj, U, V>>(g, o,
						*c, integer_cast<int32>(target.selected), value, cache);
				}
				else
				{
					string empty{};
					g.input("edit"sv, empty, gui::input_text_flags::readonly);
				}

				if (g.button("add"sv))
				{
					auto iter = std::begin(container);

					// insert after currently selected item
					if (!std::empty(container))
					{
						++target.selected;
						std::advance(iter, target.selected);
					}

					container.emplace(iter);
					if constexpr (use_object_type)
						set_curve(o, *c, container);
					ret = true;
				}

				g.layout_horizontal();

				if (g.button("remove"sv) && !std::empty(value))
				{
					auto iter = std::begin(container);
					std::advance(iter, target.selected);
					container.erase(iter);
					if constexpr (use_object_type)
						set_curve(o, *c, container);
					if (std::empty(container))
						target.selected = 0;
					else
						target.selected = std::clamp(target.selected, { 0 }, std::size(container) - 1);
					ret = true;
				}

				if (g.button("move up"sv) && target.selected > 0)
				{
					auto at = std::begin(container);
					std::advance(at, target.selected);
					auto before = at - 1;

					std::iter_swap(before, at);
					--target.selected;
					if constexpr (use_object_type)
						set_curve(o, *c, container);
					ret = true;
				}

				if (g.button("move down"sv) && target.selected + 1 != std::size(value))
				{
					auto at = std::begin(container);
					std::advance(at, target.selected);
					auto after = at + 1;
					std::iter_swap(at, after);
					++target.selected;
					if constexpr (use_object_type)
						set_curve(o, *c, container);
					ret = true;
				}

				if (disabled)
					g.end_disabled();
			}
			g.window_end();
		}

		return ret;
	}

	template< typename T, typename U, typename Obj>
	inline void make_property_row(gui& g, Obj& o,
		resources::object::curve_obj& c, typename object_editor_ui_old<Obj, T, U>::vector_curve_edit& target,
		typename object_editor_ui_old<Obj, T, U>::cache_map& cache)
	{
		if (!resources::is_curve_valid(*c.curve_ptr, c.value))
			return;

		g.push_id(c.curve_ptr);
		std::visit([&g, &o, curve = c.curve_ptr, &target, &cache](auto&& value) {
			using Type = std::decay_t<decltype(value)>;

			if constexpr (!std::is_same_v<std::monostate, Type>)
			{
				if constexpr (resources::curve_types::is_collection_type_v<Type>)
					make_vector_property_edit<Type, T, U>(g, o, data::get_as_string(curve->id), curve, value, target, cache);
				else
					make_property_edit<object_editor_ui_old<Obj, T, U>>(g, o, data::get_as_string(curve->id), *curve, value, cache);
			}

			}, c.value);
		g.pop_id(); // curve address
		return;
	}

	template<bool Tag = false>
	static inline string get_name(const object_instance &o)
	{
		using namespace std::string_literals;
		const auto type = o.obj_type ? data::get_as_string(o.obj_type->id) : to_string(o.id);
		const auto id = to_string(o.id);
		if (!o.name_id.empty())
			return o.name_id + "("s + type + ")"s + (Tag ? "##"s + id : ""s);
		else
			return type + (Tag ? "##"s + id : ""s);
	}
}

namespace hades::obj_ui
{
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
	template<typename ObjectData, typename OnChange, typename OnRemove>
	object_editor<ObjectData, OnChange, OnRemove>::object_editor(ObjectData* d,
		OnChange on_change, OnRemove on_remove)
		: _data{ d }, _on_change{ on_change }, _on_remove{ on_remove }
	{}

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline void object_editor<ObjectData, OnChange, OnRemove>::show_object_list_buttons(gui& g)
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

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline bool object_editor<ObjectData, OnChange, OnRemove>::object_list_gui(gui& g)
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
					if (!(_data->has_curve(o, position_curve) && _data->has_curve(o, size_curve)))
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

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline void object_editor<ObjectData, OnChange, OnRemove>::object_properties(gui& g)
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

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline void object_editor<ObjectData, OnChange, OnRemove>::set_selected(object_ref_t ref) noexcept
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
		_duration_edit_cache.clear();
		return;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline auto object_editor<ObjectData, OnChange, OnRemove>::add(object_instance o) -> object_ref_t
		requires can_add_objects
	{
		const auto new_obj = _data->add(std::move(o));
		set_selected(new_obj);
		return new_obj;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline void object_editor<ObjectData, OnChange, OnRemove>::erase(object_ref_t ref)
	{
		if constexpr (on_remove_callback)
			std::invoke(_on_remove, ref);
		_data->remove(ref);
		set_selected(data_type::nothing_selected);
		return;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline void object_editor<ObjectData, OnChange, OnRemove>::_edit_name(gui& g, const object_t o)
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

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline void object_editor<ObjectData, OnChange, OnRemove>::_property_editor(gui& g)
	{
		using namespace std::string_view_literals;
		using namespace std::string_literals;
		g.checkbox("Show hidden"sv, _show_hidden);

		using namespace detail::obj_ui;
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
					_duration_edit_cache.erase(c.name);
				}
			}

			// TODO: test data_type::keyframe_editor and generate a different UI
			if (!_data->is_valid(c.curve, c.value))
				continue;

			g.push_id(c_ptr);
			std::visit([&](auto&& value) {
				using Type = std::decay_t<decltype(value)>;
				// dont show hidden props
				if (c_ptr->hidden && !_show_hidden)
					return;

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

					if (_property_row(g, c.name, c.curve, disabled, value))
					{
						_data->set_value(o, c.curve, value);

						if constexpr (on_change_callback)
							std::invoke(_on_change, o);
					}
				}
			}, c.value);
			g.pop_id(); // curve address
		}

		g.pop_id(); // entity_id
		return;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove>
	template<curve_type CurveType>
	inline bool hades::object_editor<ObjectData, OnChange, OnRemove>::
		_property_row(gui& g, std::string_view name, curve_t, bool disabled, CurveType& value)
	{
		auto ret = false;
		if (disabled)
			g.begin_disabled();
		ret = g.input(name, value);
		if (disabled)
			g.end_disabled();
		return ret;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline bool hades::object_editor<ObjectData, OnChange, OnRemove>::
		_property_row(gui& g, std::string_view name, curve_t, bool disabled, curve_types::vec2_float& value)
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

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline bool hades::object_editor<ObjectData, OnChange, OnRemove>::
		_property_row(gui& g, std::string_view name, curve_t, bool disabled, curve_types::object_ref& value)
	{
		auto ret = false;
		auto value2 = integer_cast<int>(to_value(value.id));
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

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline bool hades::object_editor<ObjectData, OnChange, OnRemove>::
		_property_row(gui& g, std::string_view name, curve_t, bool disabled, curve_types::colour& value)
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

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline bool hades::object_editor<ObjectData, OnChange, OnRemove>::
		_property_row(gui& g, std::string_view name, curve_t, bool disabled, curve_types::bool_t& value)
	{
		auto ret = false;
		if (disabled)
			g.begin_disabled();
		ret = g.checkbox(name, value);
		if (disabled)
			g.end_disabled();
		return ret;
	}

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline bool hades::object_editor<ObjectData, OnChange, OnRemove>::
		_property_row(gui& g, std::string_view name, curve_t, bool disabled, curve_types::unique& value)
	{
		auto ret = false;
		// intentional copy
		string u_string = data::get_as_string(value);
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

	template<typename ObjectData, typename OnChange, typename OnRemove>
	inline bool hades::object_editor<ObjectData, OnChange, OnRemove>::
		_property_row(gui& g, std::string_view name, curve_t, bool disabled, curve_types::time_d& value)
	{
		using namespace std::string_view_literals;
		auto ret = false;
		auto& cache = _duration_edit_cache;

		auto iter = cache.find(name);
		auto& cache_entry = iter == end(cache) ? cache[to_string(name)] : iter->second;
		if (cache_entry.edit_generation == 0)
		{
			std::tie(cache_entry.edit_buffer, cache_entry.ratio) = duration_to_string(value);
			++cache_entry.edit_generation;
		}

		auto& ratio = cache_entry.ratio;

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

		g.push_id(cache_entry.edit_generation);
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

	template<typename ObjectData, typename OnChange, typename OnRemove>
	template<curve_type CurveType>
	inline bool object_editor<ObjectData, OnChange, OnRemove>::_property_row(gui& g,
		std::string_view name, curve_t c, bool disabled, CurveType& value)
		requires curve_types::is_collection_type_v<CurveType>
	{
		using namespace std::string_view_literals;

		// create vector window
		g.push_id(name);
		auto iter = _vector_edit_windows.find(name);
		if (g.button("view"sv))
		{
			if (iter == std::end(_vector_edit_windows))
				std::tie(iter, std::ignore) = _vector_edit_windows.emplace(name, vector_edit_window{});
		}
		g.pop_id();

		enum class elem_mod {
			remove, 
			move_up,
			move_down,
			error
		};

		auto changed = false;

		if (iter != std::end(_vector_edit_windows))
		{
			auto open = true;
			
			if (g.window_begin(name, open))
			{
				auto index = vector_edit_window::nothing_selected;
				auto mod = elem_mod::error;
				const auto size = std::size(value);
				const auto max = size - 1;

				auto c_ptr = _data->get_ptr(c);

				const auto var_type = to_string(curve_collection_element_type(c_ptr->data_type));

				g.text("elements:"sv);

				if (disabled)
					g.begin_disabled();
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

					if(i == max)
						g.begin_disabled();
					if (g.button("v"sv))
					{
						index = i;
						mod = elem_mod::move_down;
					}
					if (i == max)
						g.end_disabled();
					g.same_line();

					if (g.button("remove"sv))
					{
						index = i;
						mod = elem_mod::remove;
					}

					if (_property_row(g, var_type, c, disabled, value[i]))
						changed = true;

					if (index != vector_edit_window::nothing_selected &&
						mod != elem_mod::error)
					{
						switch(mod)
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

					g.pop_id();
				}
				g.layout_horizontal();
				if (disabled)
					g.end_disabled();
			}
			g.window_end();
			

			if (!open)
				_vector_edit_windows.erase(iter);
		}

		g.same_line();
		if (disabled)
			g.begin_disabled();
		g.text(name);
		if (disabled)
			g.end_disabled();
		return changed;
	}

	/// OLD
	///====================================================
	///====================================================
	///====================================================
	/// OLD

	//TODO: handle objectlist == empty in all the editor_ui functions
	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui_old<ObjectType, OnChange, IsValidPos>::show_object_list_buttons(gui& g)
	{
		using namespace std::string_view_literals;
		using namespace std::string_literals;
		const auto &objs = resources::all_objects;

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

		if (g.button("add"sv))
		{
			const auto& o_type = objs[_next_added_object_base - 1];
			// NOTE: need to force load the object type
			data::get<resources::object>(o_type.id()); 
			add(make_instance(o_type.get()));
		}

		if (_next_added_object_base == std::size_t{})
			g.end_disabled();

		g.layout_horizontal();
		if (g.button("remove"sv) && _obj_list_selected < std::size(_data->objects))
			_erase(_obj_list_selected);
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui_old<ObjectType, OnChange, IsValidPos>::object_list_gui(gui& g)
	{
		using namespace std::string_view_literals;
		if constexpr (visual_editor)
		{
			// only list the invisible objects for the visual editor
			const auto position_curve = get_position_curve();
			const auto size_curve = get_size_curve();
			if (g.listbox_begin("##obj_list"sv))
			{
				const auto siz = size(_data->objects);
				for (auto i = std::size_t{}; i < siz; ++i)
				{
					auto& o = _data->objects[i];
					if ( !(has_curve(o, *position_curve) && has_curve(o, *size_curve)) )
					{
						if (g.selectable(detail::obj_ui::get_name<true>(o), _obj_list_selected == i))
							_set_selected(i);
					}
				}
				g.listbox_end();
			}
		}
		else
		{
			if (g.listbox("##obj_list"sv, _obj_list_selected, _data->objects, detail::obj_ui::get_name<true>))
			{
				_set_selected(_obj_list_selected);
			}
		}
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui_old<ObjectType, OnChange, IsValidPos>::object_properties(gui& g)
	{
		using namespace std::string_view_literals;
		if (_selected == bad_entity)
		{
			_vector_curve_edit = {};
			g.text("Nothing is selected..."sv);
			return;
		}

        g.push_id(integer_cast<int32>(static_cast<entity_id::value_type>(_selected)));
		_property_editor(g);
		g.pop_id();
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui_old<ObjectType, OnChange, IsValidPos>::set_selected(entity_id id) noexcept
	{
		const auto index = _get_obj(id);
		_set_selected(index);
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline ObjectType* object_editor_ui_old<ObjectType, OnChange, IsValidPos>::get_obj(entity_id id) noexcept
	{
		const auto index = _get_obj(id);
		if (index < std::size(_data->objects))
			return _get_obj(index);
		return nullptr;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline const ObjectType* object_editor_ui_old<ObjectType, OnChange, IsValidPos>::get_obj(entity_id id) const noexcept
	{
		const auto index = _get_obj(id);
		if (index < std::size(_data->objects))
			return &_data->objects[index];
		return nullptr;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline entity_id object_editor_ui_old<ObjectType, OnChange, IsValidPos>::add(ObjectType o)
	{
		assert(o.id == bad_entity);
		const auto id = o.id = post_increment(_data->next_id);
		_data->objects.emplace_back(std::move(o));
		return id;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui_old<ObjectType, OnChange, IsValidPos>::erase(const entity_id id)
	{
		const auto index = _get_obj(id);
		_erase(index);
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui_old<ObjectType, OnChange, IsValidPos>::_add_remove_curve_window(gui& g)
	{
		using namespace std::string_literals;
		using namespace std::string_view_literals;
		using window = add_remove_curve_window;
		auto& w = _add_remove_window_state;
		auto open = true;

		assert(w.state != window::window_state::add);
		if (w.state == window::window_state::remove)
		{
			if (g.window_begin("reset curve"sv, open))
			{
				const auto o = _get_obj(_obj_list_selected);
				auto &curves = _curves;
				assert(w.list_index < std::size(curves));
				auto &curve = curves[w.list_index];
				const auto& c = curve.curve_ptr;
				assert(c);

				const auto selected_disabled = c->locked || c->frame_style == keyframe_style::const_t;

				if (selected_disabled)
					g.begin_disabled();

				if (g.button("reset"sv))
				{
					auto val = resources::object_functions::get_curve(*o->obj_type, *c);
					set_curve(*o, c->id, val);
					curve.value = std::move(val);
					open = false;
				}

				if (selected_disabled)
					g.end_disabled();

				g.layout_horizontal();
				if (g.button("cancel"sv))
					open = false;

				g.text("data type: "s + to_string(c->data_type));
				const auto& value = curve.value;
				if(resources::is_set(value))
					g.text("current value: "s + curve_to_string(*c, value));
				g.text("default value: "s + to_string(*c));

				// TODO: new listbox, add disabled
				g.listbox("##curve_list", w.list_index, curves, [](auto&& c)->string {
					return to_string(c.curve_ptr->id);
				});
			}
			g.window_end();
		}

		if (open == false)
			_reset_add_remove_curve_window();
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui_old<ObjectType, OnChange, IsValidPos>::_erase(const std::size_t index)
	{
		assert(index < std::size(_data->objects));
        const auto iter = std::next(std::begin(_data->objects), signed_cast(index));

		_data->entity_names.erase(iter->name_id);
		_data->objects.erase(iter);

		if (std::empty(_data->objects))
			_set_selected(index);
		else if (_obj_list_selected == std::size(_data->objects))
			_set_selected(_obj_list_selected - 1);
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	template<typename MakeRect>
	inline void object_editor_ui_old<ObjectType, OnChange, IsValidPos>::_positional_property_field(
		gui& g, std::string_view label,	ObjectType& o, curve_info& c,
		MakeRect&& make_rect)
	{
		static_assert(std::is_invocable_r_v<rect_float, MakeRect, const ObjectType&, const curve_info&>,
			"MakeRect must have the following definition: (const object_instance&, const curve_info&)->rect_float");

		const auto disabled = c.curve->locked || c.curve->frame_style == keyframe_style::const_t;

		const auto v = std::get<vector_float>(c.value);
		auto value = std::array{ v.x, v.y };
		if (disabled)
			g.begin_disabled();

		if (g.input(label, value))
		{
			c.value = vector_float{ value[0], value[1] };
			
			// TODO: is_valid_pos and on_change are only used here
			//		but position and size should always be disabled
			//		so we could remove these and the template arguments from
			//		the obj_ui class
			const auto rect = std::invoke(std::forward<MakeRect>(make_rect), o, c);
			const auto safe_pos = std::invoke(_is_valid_pos, rect, o);

			if (safe_pos)
			{
				set_curve(o, *c.curve, c.value);
				std::invoke(_on_change, o);
			}
		}

		if (disabled)
			g.end_disabled();
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline std::size_t object_editor_ui_old<ObjectType, OnChange, IsValidPos>::_get_obj(entity_id id) const noexcept
	{
		const auto size = std::size(_data->objects);
		for (auto i = std::size_t{}; i < size; ++i)
		{
			if (_data->objects[i].id == id)
				return i;
		}
		return std::numeric_limits<std::size_t>::max();
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline ObjectType* object_editor_ui_old<ObjectType, OnChange, IsValidPos>::_get_obj(std::size_t index) noexcept
	{
		assert(index < std::size(_data->objects));
		return &_data->objects[index];
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui_old<ObjectType, OnChange, IsValidPos>::_reset_add_remove_curve_window() noexcept
	{
		_add_remove_window_state = add_remove_curve_window{}; 
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui_old<ObjectType, OnChange, IsValidPos>::_property_editor(gui& g)
	{
		using namespace detail::obj_ui;
		const auto o = _get_obj(_obj_list_selected);
		const auto name = detail::obj_ui::get_name(*o);

		using namespace std::string_view_literals;
		using namespace std::string_literals;
		g.text("Selected: "s + name);
		g.push_id(integer_cast<int32>(to_value(_selected)));

		//properties
		//immutable object id
		auto id_str = to_string(o->id);
		g.begin_disabled();
		g.input_text("id"sv, id_str);
		g.end_disabled();
		make_name_id_property(g, *o, _entity_name_id_uncommited, _data->entity_names);
		g.text("curves:"sv);
		// NOTE(steven): we dont allow adding loose curves to object instances
		//	attached curves is defined by the object type
		// 
		//if (g.button("add"sv)) // dont show the 'add' button if thir are no addable curves
		//{
		//	_reset_add_remove_curve_window();
		//	_add_remove_window_state.state = add_remove_curve_window::window_state::add;
		//}

		if (!std::empty(o->curves))
		{
			//if(global_curves) //only horizontal layout if the 'add' button was also present
			//	g.layout_horizontal();

			if (g.button("reset curve"sv))
			{
				_reset_add_remove_curve_window();
				_add_remove_window_state.state = add_remove_curve_window::window_state::remove;
			}
		}

		_add_remove_curve_window(g);

		//positional properties
		if constexpr (visual_editor)
		{
			//position
			auto& pos = _curve_properties[enum_type(curve_index::pos)];
			if (resources::is_set(pos.value))
			{
				_positional_property_field(g, "position"sv, *o, pos, [](const ObjectType& o, const curve_info& c)->rect_float {
					const auto size = get_size(o);
					const auto pos = std::get<vector_float>(c.value);
					return { pos, size };
					});

				//test for collision
				const auto p = get_position(*o);
				if (p != std::get<vector_float>(pos.value))
				{
					g.tooltip("The value of position would cause an collision"sv);
					pos.value = p;
				}
			}

			auto& siz = _curve_properties[enum_type(curve_index::size_index)];
			if (resources::is_set(siz.value))
			{
				_positional_property_field(g, "size"sv, *o, siz, [](const ObjectType& o, const curve_info& c)->rect_float {
					const auto size = std::get<vector_float>(c.value);
					const auto pos = get_position(o);
					return { pos, size };
				});

				//test for collision
				const auto s = get_size(*o);
				if (s != std::get<vector_float>(siz.value))
				{
					g.tooltip("The value of size would cause an collision"sv);
					siz.value = s;
				}
			}
		}

		//other properties
		for (auto& c : _curves)
			make_property_row<OnChange, IsValidPos>(g, *o, c, _vector_curve_edit, _edit_cache);

		g.pop_id(); // entity_id
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui_old<ObjectType, OnChange, IsValidPos>::_set_selected(std::size_t index)
	{
		_reset_add_remove_curve_window();
		_vector_curve_edit = vector_curve_edit{};

		if (index >= std::size(_data->objects))
		{
			_selected = bad_entity;
			_obj_list_selected = std::size_t{};
			return;
		}

		const auto& o = _data->objects[index];
		_selected = o.id;
		_obj_list_selected = index;
		const auto pos_curve = get_position_curve();
		const auto siz_curve = get_size_curve();

		_curve_properties = std::array{
			has_curve(o, *pos_curve) ? curve_info{pos_curve, get_curve(o, *pos_curve)} : curve_info{},
			has_curve(o, *siz_curve) ? curve_info{siz_curve, get_curve(o, *siz_curve)} : curve_info{}
		};

		// stash the objects curve list
		auto all_curves = get_all_curves(o);
		if constexpr (visual_editor)
		{
			all_curves.erase(std::remove_if(begin(all_curves), end(all_curves),
				[others = _curve_properties](auto&& c) {
					return std::any_of(begin(others),
						end(others), [&c](auto&& curve)
						{ return c.curve_ptr == curve.curve; });
				}), end(all_curves));
		}

		std::sort(begin(all_curves), end(all_curves), [](auto&& lhs, auto&& rhs) {
			return data::get_as_string(lhs.curve_ptr->id) < data::get_as_string(rhs.curve_ptr->id);
			});

		_curves = std::move(all_curves);

		_entity_name_id_uncommited = o.name_id;
		_edit_cache = {}; // reset the edit cache
	}
}
