#include "hades/level_editor_objects.hpp"

#include "hades/exceptions.hpp"
#include "hades/gui.hpp"

namespace hades::detail::obj_ui
{
	//TODO: is this worth moving to the util lib
	template<std::size_t Length>
	static string clamp_length(std::string_view str)
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
				g.show_tooltip("This name is already being used"sv);
		}
		/*else if (name_reason == reason::reserved_name)
			g.show_tooltip("This name is reserved"sv);*/
		return;
	}

	template<typename T>
	static void make_property_edit(gui& g, object_instance& o, std::string_view name, const resources::curve& c, const T& value)
	{
		auto edit_value = value;
		if (g.input(name, edit_value))
			set_curve(o, c, edit_value);
	}

	template<>
	static inline void make_property_edit<entity_id>(gui& g, object_instance& o, std::string_view name, const resources::curve& c, const entity_id& value)
	{
		auto value2 = static_cast<entity_id::value_type>(value);
		make_property_edit(g, o, name, c, value2);
		const auto ent_value = entity_id{ value2 };
		//FIXME: call set_curve
	}

	template<>
	static inline void make_property_edit<bool>(gui& g, object_instance& o, std::string_view name, const resources::curve& c, const bool& value)
	{
		using namespace std::string_view_literals;
		constexpr auto tru = "true"sv;
		constexpr auto fal = "false"sv;
		if (g.combo_begin(name, value ? tru : fal))
		{
			bool true_opt = value;
			bool false_opt = !true_opt;

			if (g.selectable(tru, value))
				set_curve(o, c, true);
			if (g.selectable(fal, !value))
				set_curve(o, c, false);

			g.combo_end();
		}
	}

	template<>
	static inline void make_property_edit<string>(gui& g, object_instance& o, std::string_view name, const resources::curve& c, const string& value)
	{
		auto edit = value;
		if (g.input_text(name, edit))
			set_curve(o, c, edit);
	}

	template<>
	static inline void make_property_edit<unique_id>(gui& g, object_instance& o, std::string_view name, const resources::curve& c, const unique_id& value)
	{
		auto u_string = data::get_as_string(value);
		if (g.input_text(name, u_string))
			set_curve(o, c, data::make_uid(u_string));
	}

	template<>
	static inline void make_property_edit<vector_float>(gui& g, object_instance& o, std::string_view name, const resources::curve& c, const vector_float& value)
	{
		auto editval = std::array{ value.x, value.y };
		if (g.input(name, editval))
			set_curve(o, c, vector_float{ editval[0], editval[1] });
	}

	//TODO: clean these three specialisations up to get rid of repeated code
	template<typename T>
	static void make_vector_edit_field(gui& g, object_instance& o, const resources::curve& c, int32 selected, const T& value)
	{
		auto iter = std::cbegin(value);
		std::advance(iter, selected);

		auto edit = *iter;

		g.input("edit"sv, edit);

		if (edit != *iter)
		{
			auto container = value;
			auto iter = std::begin(container);
			std::advance(iter, selected);
			*iter = edit;
			set_curve(o, c, container);
		}
	}

	template<>
	static inline void make_vector_edit_field<resources::curve_types::collection_unique>
		(gui& g, object_instance& o, const resources::curve& c, int32 selected,
			const resources::curve_types::collection_unique& value)
	{
		using namespace std::string_view_literals;
		auto iter = std::cbegin(value);
		std::advance(iter, selected);

		auto edit = data::get_as_string(*iter);

		g.input("edit"sv, edit);

		auto id = data::make_uid(edit);

		if (id != *iter)
		{
			auto container = value;
			auto iter = std::begin(container);
			std::advance(iter, selected);
			*iter = id;
			set_curve(o, c, container);
		}
	}

	template<>
	static inline void make_vector_edit_field
		<resources::curve_types::collection_object_ref>(gui& g, object_instance& o,
			const resources::curve& c, int32 selected,
			const resources::curve_types::collection_object_ref& value)
	{
		using namespace std::string_view_literals;
		auto iter = std::begin(value);
		std::advance(iter, selected);

		auto edit = static_cast
			<resources::curve_types::object_ref::value_type>(*iter);

		g.input("edit"sv, edit);

		const auto new_val = resources::curve_types::object_ref{ edit };

		if (new_val != *iter)
		{
			auto container = value;
			auto iter = std::begin(container);
			std::advance(iter, selected);
			*iter = new_val;

			set_curve(o, c, container);
		}
	}


	template<typename T, typename Obj>
	static void make_vector_property_edit(gui& g, object_instance& o, std::string_view name,
		const resources::curve* c, const T& value, typename object_editor_ui<Obj>::vector_curve_edit& target)
	{
		using namespace std::string_view_literals;

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
					make_vector_edit_field(g, o, *c, integer_cast<int32>(target.selected), value);
				else
				{
					string empty{};
					g.input("edit"sv, empty, gui::input_text_flags::readonly);
				}

				if (g.button("add"sv))
				{
					auto container = value;
					auto iter = std::begin(container);
					++target.selected;
					std::advance(iter, target.selected);

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

	template<typename Obj>
	static inline void make_property_row(gui& g, object_instance& o,
		const resources::object::curve_obj& c, typename object_editor_ui<Obj>::vector_curve_edit& target)
	{
		const auto [curve, value] = c;

		if (!resources::is_curve_valid(*curve, value))
			return;

		std::visit([&g, &o, &curve, &target](auto&& value) {
			using T = std::decay_t<decltype(value)>;

			if constexpr (!std::is_same_v<std::monostate, T>)
			{
				if constexpr (resources::curve_types::is_collection_type_v<T>)
					make_vector_property_edit(g, o, data::get_as_string(curve->id), curve, value, target);
				else
					make_property_edit(g, o, data::get_as_string(curve->id), *curve, value);
			}

			}, value);
	}
}

namespace hades
{
	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::object_properties(gui& g)
	{
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
		_selected = id;
		if (_selected == bad_entity)
			return;
		const auto o = get_obj(id);
		assert(o);
		const auto pos_curve = get_position_curve();
		const auto siz_curve = get_size_curve();
		const auto posc = get_curve(*o, *pos_curve);
		const auto sizc = get_curve(*o, *siz_curve);

		_curve_properties = std::array{
			curve_info{pos_curve, posc},
			has_curve(*o, *siz_curve) ? curve_info{siz_curve, sizc} : curve_info{}
		};

		_entity_name_id_uncommited = o->name_id;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline ObjectType* object_editor_ui<ObjectType, OnChange, IsValidPos>::get_obj(entity_id id) noexcept
	{
		for (auto& o : _data->objects)
		{
			if (o.id == id)
				return &o;
		}

		return nullptr;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::erase(entity_id id)
	{
		//we assume the the object isn't duplicated in the object list
		const auto obj = std::find_if(std::begin(_data->objects), std::end(_data->objects), [id](auto&& o) {
			return o.id == id;
		});

		_data->entity_names.erase(obj->name_id);
		_data->objects.erase(obj);
		return;
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	template<typename MakeRect>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::_positional_property_field(
		gui& g, std::string_view label,	ObjectType& o, curve_info& c,
		MakeRect make_rect)
	{
		static_assert(std::is_invocable_r_v<rect_float, MakeRect, const ObjectType&, const curve_info&>,
			"MakeRect must have the following definition: (const object_instance&, const curve_info&)->rect_float");

		const auto v = std::get<vector_float>(c.value);
		auto value = std::array{ v.x, v.y };
		if (g.input(label, value))
		{
			c.value = vector_float{ value[0], value[1] };
			//TODO: must use both collision systems here
			const auto rect = std::invoke(make_rect, o, c);
			const auto safe_pos = std::invoke(_is_valid_pos, position(rect), size(rect), o);

			//TODO: test safe_pos against map limits
			if (safe_pos || _allow_intersect)
			{
				set_curve(o, c.curve->id, c.value);
				std::invoke(_on_change, o);
				_update_quad_data(o);
				update_object_sprite(o, _sprites);
			}
		}
	}

	template<typename ObjectType, typename OnChange, typename IsValidPos>
	inline void object_editor_ui<ObjectType, OnChange, IsValidPos>::_property_editor(gui& g)
	{
		using namespace detail::obj_ui;
		const auto o = get_obj(_selected);
		assert(o);

		const auto name = [&o] {
			assert(o->obj_type);
			using namespace std::string_literals;
			const auto type = data::get_as_string(o->obj_type->id);
			constexpr auto max_length = 15u;
			if (!o->name_id.empty())
				return clamp_length<max_length>(o->name_id) + "("s + clamp_length<max_length>(type) + ")"s;
			else
				return clamp_length<max_length>(type);
		}();

		using namespace std::string_view_literals;
		using namespace std::string_literals;
		g.text("Selected: "s + name);

		//properties
		//immutable object id
		g.input_text("id"sv, to_string(static_cast<entity_id::value_type>(o->id)), gui::input_text_flags::readonly);
		make_name_id_property(g, *o, _entity_name_id_uncommited, _data->entity_names);
	}
}
