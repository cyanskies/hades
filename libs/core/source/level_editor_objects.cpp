#include "hades/level_editor_objects.hpp"

#include "hades/animation.hpp"
#include "hades/core_curves.hpp"
#include "hades/gui.hpp"
#include "hades/parser.hpp"

using namespace std::string_view_literals;
constexpr auto level_editor_object_resource_name = "level-editor-object-settings"sv;
static auto object_settings_id = hades::unique_id::zero;

namespace hades::resources
{
	static inline void parse_level_editor_object_resource(unique_id mod, const data::parser_node &node, data::data_manager &d)
	{
		//level-editor-object-settings:
		//    object-groups:
		//        group-name: [elms, elms]

		if (object_settings_id == unique_id::zero)
			object_settings_id = unique_id{};

		auto settings = d.find_or_create<level_editor_object_settings>(object_settings_id, mod);

		const auto object_groups = node.get_child("object-groups"sv);
		if (object_groups)
		{
			const auto o_groups = object_groups->get_children();
			for (const auto &g : o_groups)
			{
				const auto name = g->to_string();
				auto iter = std::find_if(std::begin(settings->groups), std::end(settings->groups), [&name](const auto &t) {
					return std::get<string>(t) == name;
				});

				auto &group = iter != std::end(settings->groups) ? *iter : settings->groups.emplace_back();

				std::get<string>(group) = name;
				auto &group_content = std::get<std::vector<const object*>>(group);

				group_content = g->merge_sequence(std::move(group_content), [&d, mod](const std::string_view str_id) {
					const auto id = d.get_uid(str_id);
					return d.find_or_create<object>(id, mod);
				});
			}
		}//!object_groups
	}
}

namespace hades
{
	void register_level_editor_object_resources(data::data_manager &d)
	{
		d.register_resource_type(level_editor_object_resource_name, resources::parse_level_editor_object_resource);
	}

	level_editor_objects::level_editor_objects()
	{
		if (object_settings_id != unique_id::zero)
			_settings = data::get<resources::level_editor_object_settings>(object_settings_id);

		for (const auto o : resources::all_objects)
		{
			if (!o->loaded)
				data::get<resources::object>(o->id);
		}
	}

	constexpr auto quad_bucket_limit = 5;

	void level_editor_objects::level_load(const level &l)
	{
		_next_id = static_cast<entity_id::value_type>(l.next_id);
		_level_limit = { static_cast<float>(l.map_x),
						 static_cast<float>(l.map_y) };

		//setup the quad used for selecting objects
		_quad_selection = object_collision_tree{ rect_float{0.f, 0.f, _level_limit.x, _level_limit.y }, quad_bucket_limit };
		
		//TODO: load objects
	}

	template<typename Func>
	static void add_object_buttons(gui &g, float toolbox_width, const std::vector<const resources::object*> &objects, Func on_click)
	{
		static_assert(std::is_invocable_v<Func, const resources::object*>);

		constexpr auto button_size = vector_float{ 25.f, 25.f };
		constexpr auto button_scale_diff = 6.f;
		constexpr auto button_size_no_img = vector_float{ button_size.x + button_scale_diff,
														  button_size.y + button_scale_diff };

		const auto name_curve = get_name_curve();

		g.indent();

		auto x2 = 0.f;
		for (const auto o : objects)
		{
			const auto new_x2 = x2 + button_size.x;
			if (new_x2 < toolbox_width)
				g.layout_horizontal();
			else
				g.indent();

			auto clicked = false;
			
			string name{};
			if(has_curve(*o, *name_curve))
				name = get_name(*o);
			else
				name = data::get_as_string(o->id);

			if (const auto ico = get_editor_icon(*o); ico)
				clicked = g.image_button(*ico, button_size);	
			else
				clicked = g.button(name, button_size_no_img);

			g.tooltip(name);

			if (clicked)
				std::invoke(on_click, o);

			x2 = g.get_item_rect_max().x;
		}
	}

	static entity_id get_selected_id(const object_instance &o)
	{
		return o.id;
	}

	using vector_curve_edit = level_editor_objects::vector_curve_edit;

	void level_editor_objects::gui_update(gui &g, editor_windows&)
	{
		using namespace std::string_view_literals;
		//toolbar buttons
		g.main_toolbar_begin();

		if (g.toolbar_button("object selector"sv))
		{
			activate_brush();
			_brush_type = brush_type::object_selector;
			_held_object.reset();
		}

		if (g.toolbar_button("region selector"sv))
		{
			activate_brush();
			_brush_type = brush_type::region_selector;
		}

		g.main_toolbar_end();

		g.window_begin(editor::gui_names::toolbox);

		const auto toolbox_width = g.get_item_rect_max().x;

		if (g.collapsing_header("objects"sv))
		{
			g.checkbox("show objects"sv, _show_objects);
			g.checkbox("allow_intersection"sv, _allow_intersect);

			constexpr auto all_str = "all"sv;
			constexpr auto all_index = 0u;
			auto on_click_object = [this](const resources::object *o) {
				activate_brush();
				_brush_type = brush_type::object_place;
				_held_object = make_instance(o);
			};

			g.indent();

			if (g.collapsing_header("all"sv))
				add_object_buttons(g, toolbox_width, resources::all_objects, on_click_object);

			for (const auto &group : _settings->groups)
			{
				g.indent();

				if (g.collapsing_header(std::get<string>(group)))
					add_object_buttons(g, toolbox_width, std::get<std::vector<const resources::object*>>(group), on_click_object);
			}
		}

		if (g.collapsing_header("regions"sv))
		{

		}

		if (g.collapsing_header("properties"sv))
		{
			if (_brush_type == brush_type::object_selector
				&& _held_object)
				_make_property_editor(g);
			else if (_brush_type == brush_type::region_selector)
			{
			}
			else
			{
				g.text("Nothing is selected"sv);
				_vector_curve_edit = vector_curve_edit{};
			}
		}
		else
			_vector_curve_edit = vector_curve_edit{};

		g.window_end();

		//NOTE: this is the latest in a frame that we can call this
		// all on_* functions and other non-const functions should 
		// have already been called if appropriate
		_sprites.prepare();
	}

	static vector_float get_safe_size(const object_instance &o)
	{
		const auto size = get_size(o);
		if (size.x == 0.f || size.y == 0.f)
			return vector_float{ 8.f, 8.f };
		else
			return size;
	}

	static bool within_level(vector_float pos, vector_float size, vector_float level_size)
	{
		if (const auto br_corner = pos + size; pos.x < 0.f || pos.y < 0.f
			|| br_corner.x > level_size.x || br_corner.y > level_size.y)
			return false;
		else
			return true;
	}

	template<typename Object>
	std::variant<sf::Sprite, sf::RectangleShape> make_held_preview(vector_float pos, vector_float level_limit,
		const Object &o, const resources::level_editor_object_settings &s)
	{
		const auto size = get_safe_size(o);
		const auto obj_pos = pos;

		if(!within_level(pos, size, level_limit))
			return std::variant<sf::Sprite, sf::RectangleShape>{};

		const auto anims = get_editor_animations(o);
		if (anims.empty())
		{
			//make rect
			auto rect = sf::RectangleShape{ {size.x, size.y} };
			rect.setPosition({ obj_pos.x, obj_pos.y });

			auto col = s.object_colour;
			col.a -= col.a / 8;

			rect.setFillColor(col);
			rect.setOutlineColor(s.object_colour);
			return rect;
		}
		else
		{
			//make sprite
			const auto anim = anims[0];
			auto sprite = sf::Sprite{};
			animation::apply(*anim, time_point{}, sprite);
			const auto anim_size = vector_int{ anim->width, anim->height };
			const auto round_size = static_cast<vector_int>(size);

			if (anim_size != round_size)
			{
				const auto scale = sf::Vector2f{ size.x / static_cast<float>(anim_size.x),
												 size.y / static_cast<float>(anim_size.y) };

				sprite.setScale(scale);
			}

			sprite.setPosition({ obj_pos.x, obj_pos.y });
			auto col = sprite.getColor();

			col.a -= col.a / 8;
			sprite.setColor(col);
			return sprite;
		}
	}

	static void update_selection_rect(object_instance &o, std::variant<sf::Sprite, sf::RectangleShape> &selection_rect)
	{
		const auto position = get_position(o);
		const auto size = get_safe_size(o);

		auto selector = sf::RectangleShape{ {size.x + 2.f, size.y + 2.f} };
		selector.setPosition({ position.x - 1.f, position.y - 1.f });
		selector.setFillColor(sf::Color::Transparent);
		selector.setOutlineThickness(1.f);
		selector.setOutlineColor(sf::Color::White);

		selection_rect = std::move(selector);
	}

	void level_editor_objects::make_brush_preview(time_duration, mouse_pos pos)
	{
		switch (_brush_type)
		{
		case brush_type::object_place:
			[[fallthrough]];
		case brush_type::object_drag:
		{
			assert(_held_object);
			_held_preview = make_held_preview(pos, _level_limit, *_held_object, *_settings);
		}break;
		case brush_type::object_selector:
		{
			if (_held_object && _show_objects)
				update_selection_rect(*_held_object, _held_preview);
		}break;
		}
	}

	void level_editor_objects::draw_brush_preview(sf::RenderTarget &t, time_duration, sf::RenderStates s) const
	{
		//NOTE: always draw, we're only called when we are the active brush
		// if one of the brush types doesn't use _held_preview, then we'll need to draw conditionally
		std::visit([&t, s](auto &&p) {
			t.draw(p, s);
		}, _held_preview);
	}

	static void update_object_sprite(level_editor_objects::editor_object_instance &o, sprite_batch &s)
	{
		if (o.sprite_id == sprite_utility::sprite::bad_sprite_id)
			o.sprite_id = s.create_sprite();

		const auto position = get_position(o);
		const auto size = get_safe_size(o);
		const auto animation = get_random_animation(o);

		s.set_animation(o.sprite_id, animation, {});
		s.set_position(o.sprite_id, position);
		s.set_size(o.sprite_id, size);
	}

	static void set_selected_info(const object_instance &o, string &name_id, std::array<level_editor_objects::curve_info, 4> &curve_info)
	{
		const auto [posx_curve, posy_curve] = get_position_curve();
		const auto [sizx_curve, sizy_curve] = get_size_curve();
		const auto posx = get_curve(o, *posx_curve);
		const auto posy = get_curve(o, *posy_curve);
		const auto sizx = get_curve(o, *sizx_curve);
		const auto sizy = get_curve(o, *sizy_curve);

		using curve_t = level_editor_objects::curve_info;

		curve_info = std::array{
			curve_t{posx_curve, posx},
			curve_t{posy_curve, posy},
			has_curve(o, *sizx_curve) ? curve_t{sizx_curve, sizx} : curve_t{},
			has_curve(o, *sizy_curve) ? curve_t{sizy_curve, sizy} : curve_t{}
		};

		name_id = o.name_id;
	}

	static entity_id object_at(level_editor_objects::mouse_pos pos, 
		const level_editor_objects::object_collision_tree &quads)
	{
		const auto target = rect_float{ {pos.x - .5f, pos.y - .5f}, {.5f, .5f} };
		const auto rects = quads.find_collisions(target);

		for (const auto &o : rects)
		{
			if (intersects(o.rect, target))
				return o.key;
		}

		return bad_entity;
	}

	void level_editor_objects::on_click(mouse_pos p)
	{
		assert(_brush_type != brush_type::object_drag);

		//if grid snap enabled
		const auto pos = p;

		if (_brush_type == brush_type::object_selector
			&& _show_objects 
			&& within_level(p, vector_float{}, _level_limit))
		{
			const auto id = object_at(pos, _quad_selection);
			if (id == bad_entity) // nothing under cursor
				return;

			const auto obj = std::find_if(std::cbegin(_objects), std::cend(_objects), [id](auto &&o) {
				return o.id == id;
			});

			//object was in quadmap but not objectlist
			assert(obj != std::cend(_objects));

			_held_object = *obj;
			set_selected_info(*_held_object, _entity_name_id_uncommited, _curve_properties);
		}
		else if (_brush_type == brush_type::object_place)
		{
			_try_place_object(pos, *_held_object);
		}
	}

	void level_editor_objects::on_drag_start(mouse_pos pos)
	{
		if (!within_level(pos, vector_float{}, _level_limit))
			return;

		if (_brush_type == brush_type::object_selector
			&& _show_objects)
		{
			const auto id = object_at(pos, _quad_selection);

			if (id == bad_entity)
				return;

			const auto obj = std::find_if(std::cbegin(_objects), std::cend(_objects), [id](auto &&o) {
				return id == o.id;
			});

			assert(obj != std::cend(_objects));
			_held_object = *obj;

			_brush_type = brush_type::object_drag;
		}
	}

	void level_editor_objects::on_drag(mouse_pos pos)
	{
		if (!_show_objects)
			return;

		if (_brush_type == brush_type::object_place)
		{
			assert(_held_object);
			_try_place_object(pos, *_held_object);
		}

		//TODO: if object selector_selection rect stretch the rect

	}

	void level_editor_objects::on_drag_end(mouse_pos pos)
	{
		if (_brush_type == brush_type::object_drag)
		{
			if (_try_place_object(pos, *_held_object))
			{
				//object is placed at the back of the object list,
				//we need to remove the old entry
				//NOTE: _try_place updates the quadtree and 
				//sprite data for us already
				const auto obj = std::find_if(std::cbegin(_objects), std::cend(_objects), [id = _held_object->id](auto &&o) {
					return o.id == id;
				});

				_objects.erase(obj);

				_held_object = _objects.back();
				update_selection_rect(*_held_object, _held_preview);
			}

			_brush_type = brush_type::object_selector;
		}
	}

	void level_editor_objects::draw(sf::RenderTarget &t, time_duration, sf::RenderStates s) const
	{
		if(_show_objects)
			t.draw(_sprites, s);
		
		//if (_show_regions)
		//	;
		//TODO: draw regions
	}

	template<std::size_t Length>
	static string clamp_length(std::string_view str)
	{
		if (str.length() <= Length)
			return to_string(str);

		using namespace std::string_literals;
		return to_string(str.substr(0, Length)) + "..."s;
	}

	static void make_name_id_property(gui &g, object_instance &o, string &text, std::unordered_map<string, entity_id> &name_map)
	{
		if (g.input_text("Name_id"sv, text))
		{
			//if the new name is empty, and the old name isn't
			if (text == string{}
				&& !o.name_id.empty())
			{
				//remove name_id
				auto begin = std::cbegin(name_map);
				const auto end = std::cend(name_map);
				for (begin; begin != std::end(name_map); ++begin)
				{
					if (begin->second == o.id)
						break;
				}

				name_map.erase(begin);
				o.name_id.clear();
			}
			else if (const auto iter = name_map.find(text);
				iter == std::end(name_map))
			{
				//remove current name binding if present
				if (!o.name_id.empty())
				{
					for (auto begin = std::begin(name_map); begin != std::end(name_map); ++begin)
					{
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
		}

		//if the editbox is different to the current name,
		//then something must have stopped it from being commited
		if (text != o.name_id)
		{
			if (const auto iter = name_map.find(text); iter->second != o.id)
				g.show_tooltip("This name is already being used");
		}
	}

	static rect_float get_bounds(object_instance &o)
	{
		return { get_position(o), get_size(o) };
	}

	template<typename T>
	static void make_property_edit(gui &g, object_instance &o, std::string_view name, const resources::curve &c, const T &value)
	{
		auto edit_value = value;
		if (g.input(name, edit_value))
			set_curve(o, c, edit_value);
	}

	template<>
	static void make_property_edit<entity_id>(gui &g, object_instance &o, std::string_view name, const resources::curve &c, const entity_id &value)
	{
		auto value2 = static_cast<entity_id::value_type>(value);
		make_property_edit(g, o, name, c, value2);
		const auto ent_value = entity_id{ value2 };
	}

	template<>
	static void make_property_edit<bool>(gui &g, object_instance &o, std::string_view name, const resources::curve &c, const bool &value)
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
	static void make_property_edit<string>(gui &g, object_instance &o, std::string_view name, const resources::curve &c, const string &value)
	{
		auto edit = value;
		if (g.input_text(name, edit))
			set_curve(o, c, edit);
	}

	template<>
	static void make_property_edit<unique_id>(gui &g, object_instance &o, std::string_view name, const resources::curve &c, const unique_id &value)
	{
		auto u_string = data::get_as_string(value);
		if (g.input_text(name, u_string))
			set_curve(o, c, data::make_uid(u_string));
	}

	//TODO: clean these three specialisations up to get rid of repeated code
	template<typename T>
	static void make_vector_edit_field(gui &g, object_instance &o, const resources::curve &c, int32 selected, const T &value)
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
	static void make_vector_edit_field<resources::curve_types::vector_unique>
		(gui &g, object_instance &o, const resources::curve &c, int32 selected, 
		const resources::curve_types::vector_unique &value)
	{
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
	static void make_vector_edit_field
		<resources::curve_types::vector_object_ref>(gui &g, object_instance &o,
			const resources::curve &c, int32 selected,
			const resources::curve_types::vector_object_ref &value)
	{
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

	template<typename T>
	static void make_vector_property_edit(gui &g, object_instance &o, std::string_view name, 
		const resources::curve *c, const T &value, vector_curve_edit &target)
	{
		using namespace std::string_view_literals;

		if (g.button("edit vector..."sv))
			target.target = c;
		g.layout_horizontal();
		g.text(name);

		if (target.target == c)
		{
			if (g.window_begin("edit vector"sv,gui::window_flags::no_collapse))
			{
				if (g.button("done"sv))
					target = vector_curve_edit{};

				g.text(name);
				g.columns_begin(2u, false);
				
				g.listbox(std::string_view{}, target.selected, value);

				g.columns_next();

				assert(target.selected >= 0);
				if(static_cast<std::size_t>(target.selected) < std::size(value))
					make_vector_edit_field(g, o, *c, target.selected, value);	
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
						target.selected = std::clamp(target.selected, 0, static_cast<int32>(std::size(container)) - 1);
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

	static void make_property_row(gui &g, object_instance &o, const resources::object::curve_obj &c, vector_curve_edit &target)
	{
		const auto[curve, value] = c;
		
		if (!resources::is_curve_valid(*curve, value))
			return;

		std::visit([&g, &o, &curve, &target](auto &&value) {
			using T = std::decay_t<decltype(value)>;

			if constexpr (!std::is_same_v<std::monostate, T>)
			{
				if constexpr (resources::curve_types::is_vector_type_v<T>)
					make_vector_property_edit(g, o, data::get_as_string(curve->id), curve, value, target);
				else
					make_property_edit(g, o, data::get_as_string(curve->id), *curve, value);
			}

		}, value);
	}

	void level_editor_objects::_make_property_editor(gui &g)
	{
		assert(_held_object);
		const auto id = _held_object->id;

		auto &o = [this, id]()->editor_object_instance& {
			for (auto &o : _objects)
			{
				if (o.id == id)
					return o;
			}

			throw std::logic_error{"object was selected, but not in object list."};
		}();

		const auto name = [&o] {
			assert(o.obj_type);
			using namespace std::string_literals;
			const auto type = data::get_as_string(o.obj_type->id);
			constexpr auto max_length = 15u;
			if (!o.name_id.empty())
				return clamp_length<max_length>(o.name_id) + "("s + clamp_length<max_length>(type) + ")"s;
			else
				return clamp_length<max_length>(type);
		}();

		using namespace std::string_view_literals;
		using namespace std::string_literals;
		g.text("Selected: "s + name);

		//create the property editor

		//id
		//this is a display of the entities assigned id
		// it cannot be modified
		g.input_text("Id"sv, to_string(static_cast<entity_id::value_type>(o.id)), gui::input_text_flags::readonly);
		
		//name_id
		//this is the unique name that systems can use to access this entity
		// can be useful for players the world or other significant objects
		make_name_id_property(g, o, _entity_name_id_uncommited, _entity_names);

		auto &pos_x = _curve_properties[curve_index::pos_x];
		auto &pos_y = _curve_properties[curve_index::pos_y];

		_make_positional_property_edit_field(g, "x position"sv, o, pos_x,
			[](const auto &o, const auto&c) {
			auto rect = rect_float{ get_position(o), get_size(o) }; 
			rect.x = std::get<float>(c.value);
			return rect;
		}, [](auto &&o, const auto &r) {
			set_position(o, { r.x, r.y });
		});

		if (const auto pos = get_position(o); std::get<float>(pos_x.value) != pos.x)
		{
			g.show_tooltip("The value of x position would cause an collision");
			pos_x.value = pos.x;
		}

		_make_positional_property_edit_field(g, "y position"sv, o, pos_y,
			[](const auto &o, const auto&c) {
			auto rect = rect_float{ get_position(o), get_size(o) };
			rect.y = std::get<float>(c.value);
			return rect;
		}, [](auto &&o, const auto &r) {
			set_position(o, { r.x, r.y });
		});

		if (const auto pos = get_position(o); std::get<float>(pos_y.value) != pos.y)
		{
			g.show_tooltip("The value of y position would cause an collision");
			pos_y.value = pos.y;
		}

		//size
		auto &siz_x = _curve_properties[curve_index::size_x];
		auto &siz_y = _curve_properties[curve_index::size_y];

		if (const auto props_end = std::end(_curve_properties); siz_x.curve && siz_y.curve)
		{
			_make_positional_property_edit_field(g, "x size"sv, o, siz_x,
				[](const auto &o, const auto&c) {
				auto rect = rect_float{ get_position(o), get_size(o) };
				rect.width = std::get<float>(c.value);
				return rect;
			}, [](auto &&o, const auto &r) {
				set_size(o, { r.width, r.height });
			});

			if (const auto siz = get_size(o); std::get<float>(siz_x.value) != siz.x)
			{
				g.show_tooltip("The value of x size would cause an collision");
				siz_x.value = siz.x;
			}
			
			_make_positional_property_edit_field(g, "y size"sv, o, siz_y,
				[](const auto &o, const auto&c) {
				auto rect = rect_float{ get_position(o), get_size(o) };
				rect.height = std::get<float>(c.value);
				return rect;
			}, [](auto &&o, const auto &r) {
				set_size(o, { r.width, r.height });
			});

			if (const auto siz = get_size(o); std::get<float>(siz_y.value) != siz.y)
			{
				g.show_tooltip("The value of y size would cause an collision");
				siz_y.value = siz.y;
			}
		}
		
		const auto all_curves = get_all_curves(o);
		using curve_type = const resources::curve*;
		for (auto &c : all_curves)
		{
			if (std::none_of(std::begin(_curve_properties),
				std::end(_curve_properties), [&c](auto &&curve) 
			{ return std::get<curve_type>(c) == curve.curve; }))
				make_property_row(g, o, c, _vector_curve_edit);
		}

		_held_object = o;
	}

	bool level_editor_objects::_object_valid_location(const rect_float &r, const object_instance &o) const
	{
		const auto id = o.id;

		const auto collision_groups = get_collision_groups(o);
		const auto &current_groups = _collision_quads;

		if(!within_level(position(r), size(r), _level_limit))
			return false;

		//for each group that this object is a member of
		//check each neaby rect for a collision
		//return true if any such collision would occur
		const auto object_collision = std::any_of(std::begin(collision_groups), std::end(collision_groups),
			[&current_groups, &r, id](const unique_id cg) {
			const auto group_quadtree = current_groups.find(cg);
			if (group_quadtree == std::end(current_groups))
				return false;

			const auto local_rects = group_quadtree->second.find_collisions(r);

			return std::any_of(std::begin(local_rects), std::end(local_rects),
				[&r, id](const quad_data<entity_id, rect_float> &other) {
				return other.key != id && intersects(other.rect, r);
			});
		});

		//TODO: a way to ask the terrain system for it's say on this object?
		return !object_collision;
	}

	bool level_editor_objects::_object_valid_location(vector_float pos, vector_float size, const object_instance &o) const
	{
		return _object_valid_location({ pos, size }, o);
	}

	void level_editor_objects::_remove_object(entity_id id)
	{
		//NOTE: we only remove the first element to match,
		//this allows other functions to add a replacement to the end
		// of the list before calling this.
		const auto obj = std::find_if(std::begin(_objects), std::end(_objects), [id](auto &&o) {
			return o.id == id;
		});
	}

	bool level_editor_objects::_try_place_object(vector_float pos, editor_object_instance o)
	{
		const auto size = get_size(o);

		const auto new_entity = o.id == bad_entity;

		if(new_entity)
			o.id = entity_id{ _next_id + 1 };
		
		const auto valid_location = _object_valid_location(pos, size, o);

		if (valid_location || _allow_intersect)
		{
			if(new_entity)
				++_next_id;

			set_position(o, pos);
			_update_quad_data(o);
			_objects.emplace_back(std::move(o));

			update_object_sprite(_objects.back(), _sprites);
			return true;
		}

		return false;
	}

	void level_editor_objects::_update_quad_data(const object_instance &o)
	{
		//update selection quad
		const auto position = get_position(o);

		{
			const auto select_size = get_safe_size(o);
			_quad_selection.insert({ position, select_size }, o.id);
		}
		
		//update collision quad
		const auto collision_groups = get_collision_groups(o);
		const auto size = get_size(o);
		const auto rect = rect_float{ position, size };

		//remove this object from all the quadtrees
		//this is needed if the object is no longer 
		//part of a specific collision group
		for (auto &group : _collision_quads)
			group.second.remove(o.id);

		//reinsert into the correct trees
		//making new trees if needed
		for (auto col_group_id : collision_groups)
		{
			//emplace creates the entry if needed, otherwise returns
			//the existing one, we don't care which
			auto[group, found] = _collision_quads.emplace(std::piecewise_construct,
				std::forward_as_tuple(col_group_id),
				std::forward_as_tuple(rect_float{ {0.f, 0.f}, _level_limit }, quad_bucket_limit));

			std::ignore = found;
			group->second.insert(rect, o.id);
		}
	}

	level_editor_objects::editor_object_instance::editor_object_instance(const object_instance &o)
		: object_instance{o}
	{}
}