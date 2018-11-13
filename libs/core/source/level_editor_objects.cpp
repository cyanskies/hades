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

	void level_editor_objects::gui_update(gui &g)
	{
		using namespace std::string_view_literals;
		//toolbar buttons
		g.main_toolbar_begin();

		if (g.toolbar_button("object selector"sv))
		{
			activate_brush();
			_brush_type = brush_type::object_selector;
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
			if (_brush_type == brush_type::object_selector)
			{
			}
			else if (_brush_type == brush_type::region_selector)
			{
			}
			else
				g.text("Nothing is selected"sv);
		}

		g.window_end();

		//NOTE: this is the latest in a frame that we can call this
		// all on_* functions and other non-const functions should 
		// have already been called if appropriate
		_sprites.prepare();
	}

	static vector_float get_safe_size(const object_instance &o)
	{
		const auto size = get_size(o);
		return size == vector_float{} ? vector_float{8.f, 8.f} : size;
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

	void level_editor_objects::on_click(mouse_pos p)
	{
		//if grid snap enabled
		const auto pos = p;

		if (_brush_type == brush_type::object_selector
			&& _show_objects && within_level(p, vector_float{}, _level_limit))
		{
			//select object under brush
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
				const auto id = _held_object->sprite_id = _sprites.create_sprite();
				//TODO: layer support
				_sprites.set_position(id, pos);
				_sprites.set_size(id, size);

				const auto anim = _held_object->obj_type->editor_anims.empty() ? nullptr
					: get_editor_animations(*_held_object)[0];

				_sprites.set_animation(id, anim, {});
				_held_object->id = entity_id{ _next_id++ };
				set_position(*_held_object, pos);
				_objects.emplace_back(*_held_object);
				_quad.insert({ pos, size }, _held_object->id);
			}
		}
	}

	void level_editor_objects::draw(sf::RenderTarget &t, time_duration, sf::RenderStates s) const
	{
		//TODO: draw objects checkbox
		if(_show_objects)
			t.draw(_sprites, s);
		
		//if (_show_regions)
		//	;
		//TODO: draw regions
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