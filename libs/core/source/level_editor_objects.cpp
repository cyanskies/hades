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

	void level_editor_objects::level_load(const level &l)
	{
		_next_id = static_cast<entity_id::value_type>(l.next_id);
		_level_limit = { static_cast<float>(l.map_x),
						 static_cast<float>(l.map_y) };

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
			
			auto name = get_name(*o); 
			if (name.empty())
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
				_vector_property_window_open = false;
			}
		}
		else
			_vector_property_window_open = false;

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
		const Object &o, const resources::level_editor_object_settings &s, const quad_tree<entity_id, rect_float> &quads)
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

	void level_editor_objects::make_brush_preview(time_duration, mouse_pos pos)
	{
		switch (_brush_type)
		{
		case brush_type::object_place:
		{
			assert(_held_object);
			_held_preview = make_held_preview(pos, _level_limit, *_held_object, *_settings, _quad);
		}break;
		case brush_type::object_selector:
		{
			if (_held_object && _show_objects)
			{
				//draw selector around object
				const auto position = get_position(*_held_object);
				const auto size = get_safe_size(*_held_object);

				auto selector = sf::RectangleShape{ {size.x + 2.f, size.y + 2.f} };
				selector.setPosition({ position.x - 1.f, position.y - 1.f });
				selector.setFillColor(sf::Color::Transparent);
				selector.setOutlineThickness(1.f);
				selector.setOutlineColor(sf::Color::White);

				_held_preview = std::move(selector);
			}
		}break;
		case brush_type::object_drag:
		{
			//draw dragged object
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
		auto info = std::array{
			curve_t{posx_curve, posx},
			curve_t{posy_curve, posy},
			curve_t{sizx_curve, sizx},
			curve_t{sizy_curve, sizy},
		};

		curve_info = info;
		name_id = o.name_id;
	}

	static void update_selection_rect(object_instance &o, std::variant<sf::Sprite, sf::RectangleShape> &selection_rect)
	{}

	void level_editor_objects::on_click(mouse_pos p)
	{
		//if grid snap enabled
		const auto pos = p;

		if (_brush_type == brush_type::object_selector
			&& _show_objects && within_level(p, vector_float{}, _level_limit))
		{
			const auto target = rect_float{ {pos.x - .5f, pos.y - .5f}, {.5f, .5f} };
			const auto rects = _quad.find_collisions(target);
			//select object under brush
			auto id = bad_entity;
			for (const auto &o : rects)
			{
				if (intersects(o.rect, target))
				{
					id = o.key;
					break;
				}
			}

			if (id == bad_entity)
				return;

			for (const auto &o : _objects)
			{
				if (o.id == id)
				{
					_held_object = o;
					set_selected_info(*_held_object, _entity_name_id_uncommited, _curve_properties);
					break;
				}
			}
		}
		else if (_brush_type == brush_type::object_place)
		{
			const auto size = get_safe_size(*_held_object);
		
			const auto bounding_rect = rect_float{ pos, size };
			const auto rects = _quad.find_collisions(bounding_rect);
			const auto intersect_found = std::any_of(std::begin(rects), std::end(rects), [bounding_rect](const auto &rect) {
				return intersects(rect.rect, bounding_rect);
			});

			if (within_level(pos, size, _level_limit) && !intersect_found || _allow_intersect)
			{
				_held_object->id = entity_id{ _next_id++ };
				set_position(*_held_object, pos);
				_objects.emplace_back(*_held_object);
				_quad.insert({ pos, size }, _held_object->id);

				update_object_sprite(_objects.back(), _sprites);
			}
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

	template<typename MakeBoundRect, typename SetChangedProperty>
	static void	make_positional_property(gui &g, std::string_view label, bool allow_intersect, level_editor_objects::editor_object_instance &o,
		level_editor_objects::curve_info &edit_curve, quad_tree<entity_id, rect_float> &quad, sprite_batch &s,
		std::variant<sf::Sprite, sf::RectangleShape> &preview, MakeBoundRect make_rect, SetChangedProperty apply)
	{
		static_assert(std::is_invocable_r_v<rect_float, MakeBoundRect, const object_instance&, const level_editor_objects::curve_info&>,
			"MakeBoundRect must have the following definition: (const object_instance&, const curve_info&)->rect_float");
		static_assert(std::is_invocable_v<SetChangedProperty, object_instance&, const rect_float&>,
			"SetChangedProperty must have the following definition (object_instance&, const rect_float&)");

		if (g.input(label, std::get<float>(edit_curve.value)))
		{
			const auto rect = std::invoke(make_rect, o, edit_curve);
			const auto others = quad.find_collisions(rect);

			const auto safe_pos = std::none_of(std::begin(others), std::end(others), [rect, id = o.id](auto &&other){
				return intersects(rect, other.rect) && id != other.key;
			});

			if (safe_pos || allow_intersect)
			{
				std::invoke(apply, o, rect);
				quad.insert(rect, o.id);
				update_object_sprite(o, s);
				update_selection_rect(o, preview);
			}
		}
	}

	template<typename T>
	static void make_property_edit(gui &g, object_instance &o, std::string_view name, const resources::curve *c, const T &value)
	{
		//g.input(name, value);
	}

	template<>
	static void make_property_edit<entity_id>(gui &g, object_instance &o, std::string_view name, const resources::curve *c, const entity_id &value)
	{
		auto value2 = static_cast<entity_id::value_type>(value);
		make_property_edit(g, o, name, c, value2);
		//value = entity_id{ value2 };
	}

	template<>
	static void make_property_edit<bool>(gui &g, object_instance &o, std::string_view name, const resources::curve *c, const bool &value)
	{
		if (g.combo_begin(name, to_string(value)))
		{
			bool true_opt, false_opt;

			true_opt = value;
			false_opt = !true_opt;

			using namespace std::string_view_literals;
			g.selectable_easy("true"sv, true_opt);
			g.selectable_easy("false"sv, false_opt);
			g.combo_end();

			//value = true_opt;
		}
	}

	template<>
	static void make_property_edit<string>(gui &g, object_instance &o, std::string_view name, const resources::curve *c, const string &value)
	{
		//g.input_text(name, value);
	}

	template<>
	static void make_property_edit<unique_id>(gui &g, object_instance &o, std::string_view name, const resources::curve *c, const unique_id &value)
	{
		auto u_string = data::get_as_string(value);
		g.input_text(name, u_string);
		//value = data::make_uid(u_string);
	}

	template<typename T>
	static void make_vector_property_edit(gui &g, object_instance &o, std::string_view name, const resources::curve *c, const T &value, bool &vector_window)
	{

	}

	static void make_property_row(gui &g, object_instance &o, const resources::object::curve_obj &c, bool &vector_window)
	{
		const auto[curve, value] = c;
		
		if (!resources::is_curve_valid(*curve, value))
			return;

		std::visit([&g, &o, &curve, &vector_window](auto &&value) {
			using T = std::decay_t<decltype(value)>;

			if constexpr (!std::is_same_v<std::monostate, T>)
			{
				if constexpr (resources::curve_types::is_vector_type_v<T>)
					make_vector_property_edit(g, o, data::get_as_string(curve->id), curve, value, vector_window);
				else
					make_property_edit(g, o, data::get_as_string(curve->id), curve, value);
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

		//get all of the other curves that we'll handle here
		const auto special_curves = std::array{
			get_name_curve(),
			std::get<0>(get_position_curve()),
			std::get<1>(get_position_curve()),
			std::get<0>(get_size_curve()),
			std::get<1>(get_size_curve())
		};

		assert(std::all_of(std::begin(special_curves), std::end(special_curves), [](auto &&c) { return c; }));

		//we show the position and size properties before the others
		//position
		// modifying these curves actively modifies the world
		//TODO: this have static indicies now, replace these with []
		auto pos_x = std::find_if(std::begin(_curve_properties), std::end(_curve_properties), [&special_curves](auto &&elm) {
			return elm.curve == special_curves[1u];
		});
		auto pos_y = std::find_if(std::begin(_curve_properties), std::end(_curve_properties), [&special_curves](auto &&elm) {
			return elm.curve == special_curves[2u];
		});

		make_positional_property(g, "x position"sv, _allow_intersect, o, *pos_x, _quad, _sprites, _held_preview,
			[](const auto &o, const auto&c) {
			auto rect = rect_float{ get_position(o), get_size(o) }; 
			rect.x = std::get<float>(c.value);
			return rect;
		}, [](auto &&o, const auto &r) {
			set_position(o, { r.x, r.y });
		});

		if (const auto pos = get_position(o); std::get<float>(pos_x->value) != pos.x)
		{
			g.show_tooltip("The value of x position would cause an collision");
			pos_x->value = pos.x;
		}

		make_positional_property(g, "y position"sv, _allow_intersect, o, *pos_y, _quad, _sprites, _held_preview,
			[](const auto &o, const auto&c) {
			auto rect = rect_float{ get_position(o), get_size(o) };
			rect.y = std::get<float>(c.value);
			return rect;
		}, [](auto &&o, const auto &r) {
			set_position(o, { r.x, r.y });
		});

		if (const auto pos = get_position(o); std::get<float>(pos_y->value) != pos.y)
		{
			g.show_tooltip("The value of y position would cause an collision");
			pos_y->value = pos.y;
		}

		//size
		auto siz_x = std::find_if(std::begin(_curve_properties), std::end(_curve_properties), [&special_curves](auto &&elm) {
			return elm.curve == special_curves[3u];
		});
		auto siz_y = std::find_if(std::begin(_curve_properties), std::end(_curve_properties), [&special_curves](auto &&elm) {
			return elm.curve == special_curves[4u];
		});

		if (const auto props_end = std::end(_curve_properties); siz_x != props_end && siz_y != props_end)
		{
			make_positional_property(g, "x size"sv, _allow_intersect, o, *siz_x, _quad, _sprites, _held_preview,
				[](const auto &o, const auto&c) {
				auto rect = rect_float{ get_position(o), get_size(o) };
				rect.width = std::get<float>(c.value);
				return rect;
			}, [](auto &&o, const auto &r) {
				set_size(o, { r.width, r.height });
			});

			if (const auto siz = get_size(o); std::get<float>(siz_x->value) != siz.x)
			{
				g.show_tooltip("The value of x size would cause an collision");
				siz_x->value = siz.x;
			}
			
			make_positional_property(g, "y size"sv, _allow_intersect, o, *siz_y, _quad, _sprites, _held_preview,
				[](const auto &o, const auto&c) {
				auto rect = rect_float{ get_position(o), get_size(o) };
				rect.height = std::get<float>(c.value);
				return rect;
			}, [](auto &&o, const auto &r) {
				set_size(o, { r.width, r.height });
			});

			if (const auto siz = get_size(o); std::get<float>(siz_y->value) != siz.y)
			{
				g.show_tooltip("The value of y size would cause an collision");
				siz_y->value = siz.y;
			}
		}
		
		const auto all_curves = get_all_curves(o);
		for (auto &c : all_curves)
			make_property_row(g, o, c, _vector_property_window_open);
	}

	void level_editor_objects::_update_position(const object_instance &o, vector_float p)
	{
		const auto s = get_size(o);
		_quad.insert({ p, s }, o.id);
	}

	void level_editor_objects::_update_size(const object_instance &o, vector_float s)
	{
		const auto p = get_position(o);
		_quad.insert({ p, s }, o.id);
	}

	void level_editor_objects::_update_pos_size(entity_id id, vector_float p, vector_float s)
	{
		_quad.insert({ p, s }, id);
	}

	level_editor_objects::editor_object_instance::editor_object_instance(const object_instance &o)
		: object_instance{o}
	{}
}