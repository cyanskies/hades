#include "hades/level_editor_objects.hpp"

#include "hades/colour.hpp"
#include "hades/core_curves.hpp"
#include "hades/curve_extra.hpp"
#include "hades/exceptions.hpp"
#include "hades/gui.hpp"

namespace hades::detail::obj_ui
{
	//TODO: is this worth moving to the util lib
	template<std::size_t Length>
	string clamp_length(std::string_view str)
	{
		if (str.length() <= Length)
			return to_string(str);

		using namespace std::string_literals;
		return to_string(str.substr(0, Length)) + "..."s;
	}

	static inline void make_name_id_property(gui& g, object_instance& o, string& text, std::unordered_map<string, entity_id>& name_map)
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
				for (begin; ; ++begin)
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

	//TODO: rename these overloads to make the usage more obvious
	// make_prop_edit is for main call
	// make_prop_edit2 is for implementing the actual property row
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

	/*struct curve_edit_cache
	{
		string edit_buffer;
		int32 edit_generation = 0;
		std::any extra_data;
	};*/

	template<typename ObjEditor>
	inline std::optional<time_duration> make_property_edit_custom(gui& g, std::string_view name, const time_duration& value,
		typename ObjEditor::cache_map& cache)
	{
		using namespace std::string_view_literals;

		auto& cache_entry = cache[to_string(name)];
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

		if constexpr (has_custom_edit_func<T>::value)
			return make_property_edit_custom<ObjEditor>(g, name, value, cache);
		else
			return make_property_edit_basic<ObjEditor>(g, name, value, cache);
	}

	template<typename ObjEditor, typename T>
	void make_property_edit(gui& g, object_instance& o, std::string_view name, const resources::curve& c, const T& value,
		typename ObjEditor::cache_map& cache)
	{
		if constexpr (resources::curve_types::is_vector_type_v<T>)
		{
			auto arr = std::array{ value.x, value.y };
			if (g.input(name, arr))
				set_curve(o, c, T{ arr[0], arr[1] });

			g.tooltip(name);
		}
		else
		{
			auto new_value = make_property_edit_impl<ObjEditor>(g, name, value, cache);
			if (new_value)
				set_curve(o, c, std::move(*new_value));
		}
	}

	template<typename ObjEditor, typename T>
	void make_vector_edit_field(gui& g, object_instance& o, const resources::curve& c, int32 selected, const T& value,
		typename ObjEditor::cache_map& cache)
	{
		using namespace std::string_view_literals;
		auto iter = std::cbegin(value);
		std::advance(iter, selected);

		const typename T::value_type& value_ref = *iter;
		auto result = make_property_edit_impl<ObjEditor>(g, "edit"sv, value_ref, cache);
		if (result)
		{
			auto container = value;
			auto target = std::begin(container);
			std::advance(target, selected);
			*target = std::move(*result);
			set_curve(o, c, std::move(container));
		}
	}

	template<typename T, typename U, typename V, typename Obj>
	void make_vector_property_edit(gui& g, Obj& o, std::string_view name,
		const resources::curve* c, const T& value, typename object_editor_ui<Obj, U, V>::vector_curve_edit& target,
		typename object_editor_ui<Obj, U, V>::cache_map& cache)
	{
		using namespace std::string_view_literals;

		using vector_curve_edit = typename object_editor_ui<Obj, U, V>::vector_curve_edit;

		if (g.button("edit vector..."sv))
			target.target = c;
		g.layout_horizontal();
		g.text(name);

		if (target.target == c)
		{
			if (g.window_begin("edit vector"sv, gui::window_flags::no_collapse))
			{
				if (g.button("done"sv))
					target = vector_curve_edit{};

				g.text(name);
				g.columns_begin(2u, false);

				g.listbox(std::string_view{}, target.selected, value);

				g.columns_next();

				assert(target.selected >= 0);
				if (static_cast<std::size_t>(target.selected) < std::size(value))
					make_vector_edit_field<object_editor_ui<Obj, U, V>>(g, o, *c, integer_cast<int32>(target.selected), value, cache);
				else
				{
					string empty{};
					g.input("edit"sv, empty, gui::input_text_flags::readonly);
				}

				if (g.button("add"sv))
				{
					auto container = value;
					auto iter = std::begin(container);

					if (!std::empty(container))
					{
						++target.selected;
						std::advance(iter, target.selected);
					}

					container.emplace(iter);
					set_curve(o, *c, container);
				}

				g.layout_horizontal();

				if (g.button("remove"sv) && !std::empty(value))
				{
					auto container = value;
					auto iter = std::begin(container);
					std::advance(iter, target.selected);
					container.erase(iter);

					set_curve(o, *c, container);
					if (std::empty(container))
						target.selected = 0;
					else
						target.selected = std::clamp(target.selected, { 0 }, std::size(container) - 1);
				}

				if (g.button("move up"sv) && target.selected > 0)
				{
					auto container = value;

					auto at = std::begin(container);
					std::advance(at, target.selected);
					auto before = at - 1;

					std::iter_swap(before, at);
					--target.selected;
					set_curve(o, *c, container);
				}

				if (g.button("move down"sv) && target.selected + 1 != std::size(value))
				{
					auto container = value;
					auto at = std::begin(container);
					std::advance(at, target.selected);
					auto after = at + 1;
					std::iter_swap(at, after);
					++target.selected;
					set_curve(o, *c, container);
				}
			}
			g.window_end();
		}
	}

	template< typename T, typename U, typename Obj>
	inline void make_property_row(gui& g, Obj& o,
		const resources::object::curve_obj& c, typename object_editor_ui<Obj, T, U>::vector_curve_edit& target,
		typename object_editor_ui<Obj, T, U>::cache_map& cache)
	{
		const auto [curve, value] = c;

		if (!resources::is_curve_valid(*curve, value))
			return;

		g.push_id(curve);
		std::visit([&g, &o, &curve, &target, &cache](auto&& value) {
			using Type = std::decay_t<decltype(value)>;

			if constexpr (!std::is_same_v<std::monostate, Type>)
			{
				if constexpr (resources::curve_types::is_collection_type_v<Type>)
					make_vector_property_edit<Type, T, U>(g, o, data::get_as_string(curve->id), curve, value, target, cache);
				else
					make_property_edit<object_editor_ui<Obj, T, U>>(g, o, data::get_as_string(curve->id), *curve, value, cache);
			}

			}, value);
		g.pop_id(); // curve address
		return;
	}

	static inline string get_name(const object_instance &o)
	{
		using namespace std::string_literals;
		const auto type = o.obj_type ? data::get_as_string(o.obj_type->id) : to_string(o.id);
		constexpr auto max_length = 15u;
		if (!o.name_id.empty())
			return clamp_length<max_length>(o.name_id) + "("s + clamp_length<max_length>(type) + ")"s;
		else
			return clamp_length<max_length>(type);
	}
}

namespace hades
{
	//TODO: handle objectlist == empty in all the editor_ui functions
	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::show_object_list_buttons(gui& g)
	{
		const auto &objs = resources::all_objects;

		assert(_next_added_object_base < std::size(objs) + 1 || std::empty(objs));

		const auto preview = [&objs](std::size_t index)->string {
			if (index == std::size_t{})
				return "none";
			else
				return to_string(objs[index - 1]->id);
		}(_next_added_object_base);

		if (g.combo_begin("object type", preview))
		{
			if (g.selectable("none", _next_added_object_base == std::size_t{}))
				_next_added_object_base = std::size_t{};

			const auto end = std::size(objs);
			for (auto i = std::size_t{}; i < end; ++i)
			{
				const auto name = to_string(objs[i]->id);
				if (g.selectable(name, _next_added_object_base == i + 1))
					_next_added_object_base = i + 1;
			}

			g.combo_end();
		}

		if (g.button("add"))
		{
			if (_next_added_object_base == std::size_t{})
				add();
			else
			{
				const auto o_type = objs[_next_added_object_base - 1];
				auto obj = ObjectType{ o_type.get() };
				add(std::move(obj));
			}
		}
		g.layout_horizontal();
		if (g.button("remove") && _obj_list_selected < std::size(_data->objects))
			_erase(_obj_list_selected);
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::object_list_gui(gui& g)
	{
		if (g.listbox("##obj_list", _obj_list_selected, _data->objects, detail::obj_ui::get_name))
		{
			_set_selected(_obj_list_selected);
		}
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::object_properties(gui& g)
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
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::set_selected(entity_id id) noexcept
	{
		const auto index = _get_obj(id);
		_set_selected(index);
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline ObjectType* object_editor_ui<ObjectType, OnChange, IsValidPos>::get_obj(entity_id id) noexcept
	{
		const auto index = _get_obj(id);
		if (index < std::size(_data->objects))
			return _get_obj(index);
		return nullptr;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline const ObjectType* object_editor_ui<ObjectType, OnChange, IsValidPos>::get_obj(entity_id id) const noexcept
	{
		const auto index = _get_obj(id);
		if (index < std::size(_data->objects))
			return &_data->objects[index];
		return nullptr;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline entity_id object_editor_ui<ObjectType, OnChange, IsValidPos>::add()
	{
		static_assert(std::is_default_constructible_v<ObjectType>);
		const auto id = post_increment(_data->next_id);
		auto o = ObjectType{};
		o.id = id;
		_data->objects.emplace_back(std::move(o));
		return id;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline entity_id object_editor_ui<ObjectType, OnChange, IsValidPos>::add(ObjectType o)
	{
		assert(o.id == bad_entity);
		const auto id = o.id = post_increment(_data->next_id);
		_data->objects.emplace_back(std::move(o));
		return id;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::erase(const entity_id id)
	{
		const auto index = _get_obj(id);
		_erase(index);
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::_add_remove_curve_window(gui& g)
	{
		using namespace std::string_literals;
		using namespace std::string_view_literals;
		using window = add_remove_curve_window;
		auto& w = _add_remove_window_state;
		auto open = true;

		if (w.state == window::window_state::add)
		{
			if (g.window_begin("add curve", open))
			{
				const auto o = _get_obj(_obj_list_selected);

				const auto curves = [&o]() {
					const auto& all_curves = resources::get_all_curves();
					auto curves = std::vector<const resources::curve*>{};
					curves.reserve(std::size(all_curves));
					std::copy_if(std::begin(all_curves), std::end(all_curves), std::back_inserter(curves),
						[&o](auto&& curve) {
							return !has_curve(*o, *curve);
						});
					return curves;
				}();

				assert(w.list_index < std::size(curves));
				const auto c = curves[w.list_index];
				assert(c);

				if(g.button("add"))
				{
					set_curve(*o, *c, {});
					open = false;
				}
				g.layout_horizontal();
				if(g.button("cancel"))
					open = false;

				g.text("data type: " + to_string(c->data_type));
				g.text("default value: " + to_string(*c));

				g.listbox("##curve_list", w.list_index, curves, [](auto&& c)->string {
					return to_string(c->id);
				});
			}
			g.window_end();
		}
		else if (w.state == window::window_state::remove)
		{
			if (g.window_begin("reset curve"sv, open))
			{
				const auto o = _get_obj(_obj_list_selected);
				const auto &curves = get_all_curves(*o);
				assert(w.list_index < std::size(curves));
				const auto &curve = curves[w.list_index];
				const auto& c = curve.curve;
				assert(c);

				if (g.button("reset"sv))
				{
					const auto iter = std::next(std::begin(o->curves), w.list_index);
					o->curves.erase(iter);
					open = false;
				}
				g.layout_horizontal();
				if (g.button("cancel"sv))
					open = false;

				g.text("data type: "s + to_string(c->data_type));
				const auto& value = curve.value;
				if(resources::is_set(value))
					g.text("current value: "s + curve_to_string(*c, value));
				g.text("default value: "s + to_string(*c));

				g.listbox("##curve_list", w.list_index, curves, [](auto&& c)->string {
					return to_string(c.curve->id);
				});
			}
			g.window_end();
		}

		if (open == false)
			_reset_add_remove_curve_window();
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::_erase(const std::size_t index)
	{
		assert(index < std::size(_data->objects));
		const auto iter = std::next(std::begin(_data->objects), index);

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
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::_positional_property_field(
		gui& g, std::string_view label,	ObjectType& o, curve_info& c,
		MakeRect&& make_rect)
	{
		static_assert(std::is_invocable_r_v<rect_float, MakeRect, const ObjectType&, const curve_info&>,
			"MakeRect must have the following definition: (const object_instance&, const curve_info&)->rect_float");

		const auto v = std::get<vector_float>(c.value);
		auto value = std::array{ v.x, v.y };
		if (g.input(label, value))
		{
			c.value = vector_float{ value[0], value[1] };
			
			const auto rect = std::invoke(std::forward<MakeRect>(make_rect), o, c);
			const auto safe_pos = std::invoke(_is_valid_pos, rect, o);

			if (safe_pos)
			{
				set_curve(o, c.curve->id, c.value);
				std::invoke(_on_change, o);
			}
		}
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline std::size_t object_editor_ui<ObjectType, OnChange, IsValidPos>::_get_obj(entity_id id) const noexcept
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
	inline ObjectType* object_editor_ui<ObjectType, OnChange, IsValidPos>::_get_obj(std::size_t index) noexcept
	{
		assert(index < std::size(_data->objects));
		return &_data->objects[index];
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::_reset_add_remove_curve_window() noexcept
	{
		_add_remove_window_state = add_remove_curve_window{}; 
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::_property_editor(gui& g)
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
		g.input_text("id"sv, id_str, gui::input_text_flags::readonly);
		make_name_id_property(g, *o, _entity_name_id_uncommited, _data->entity_names);
		g.text("curves:"sv);
		// NOTE(steven): we dont allow adding loose curves to object instances
		//	if they have a curve that is defined by the object type
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
			_positional_property_field(g, "position"sv, *o, pos, [](const ObjectType& o, const curve_info &c)->rect_float {
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
		const auto all_curves = get_all_curves(*o);
		using curve_type = const resources::curve*;
		for (auto& c : all_curves)
		{
			if constexpr (visual_editor)
			{
				if (std::none_of(std::begin(_curve_properties),
					std::end(_curve_properties), [&c](auto&& curve)
					{ return c.curve == curve.curve; }))
					make_property_row<OnChange, IsValidPos>(g, *o, c, _vector_curve_edit, _edit_cache);
			}
			else
				make_property_row<OnChange, IsValidPos>(g, *o, c, _vector_curve_edit, _edit_cache);
		}

		g.pop_id(); // entity_id
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::_set_selected(std::size_t index)
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
		const auto posc = get_curve(o, *pos_curve);
		const auto sizc = get_curve(o, *siz_curve);

		_curve_properties = std::array{
			curve_info{pos_curve, posc},
			has_curve(o, *siz_curve) ? curve_info{siz_curve, sizc} : curve_info{}
		};

		_entity_name_id_uncommited = o.name_id;
		_edit_cache = {}; // reset the edit cache
	}
}
