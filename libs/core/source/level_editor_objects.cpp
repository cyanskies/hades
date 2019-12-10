#include "hades/level_editor_objects.hpp"

#include "hades/animation.hpp"
#include "hades/background.hpp"
#include "hades/core_curves.hpp"
#include "hades/gui.hpp"
#include "hades/level_editor_grid.hpp"
#include "hades/mouse_input.hpp"
#include "hades/parser.hpp"

using namespace std::string_view_literals;
constexpr auto level_editor_object_resource_name = "level-editor-object-settings"sv;
static auto object_settings_id = hades::unique_id::zero;
constexpr auto reserved_object_names = { 
	"world" // reserved for the world object
};

namespace hades::resources
{
	static inline void parse_level_editor_object_resource(unique_id mod, const data::parser_node &node, data::data_manager &d)
	{
		//level-editor-object-settings:
		//    object-groups:
		//        group-name: [elms, elms]

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
		register_objects(d);

		d.register_resource_type(level_editor_object_resource_name, resources::parse_level_editor_object_resource);

		//create default settings object
		object_settings_id = unique_id{};
		d.find_or_create<resources::level_editor_object_settings>(object_settings_id, unique_id::zero);

	}

	level_editor_objects::level_editor_objects()
		: _obj_ui{ &_objects, [this](editor_object_instance& o) {
				return _update_changed_obj(o);
			},
			[this](const rect_float& r, const object_instance& o)->bool {
				return _object_valid_location(r, o);
			}
		}
	{
		if (object_settings_id != unique_id::zero)
			_settings = data::get<resources::level_editor_object_settings>(object_settings_id);

		for (const auto o : resources::all_objects)
		{
			if (!o->loaded)
				data::get<resources::object>(o->id);
		}
	}

	static vector_float get_safe_size(const object_instance &o)
	{
		const auto size = get_size(o);
		if (size.x == 0.f || size.y == 0.f)
			return vector_float{ 8.f, 8.f };
		else
			return size;
	}

	static void update_object_sprite(level_editor_objects::editor_object_instance &o, sprite_batch &s)
	{
		if (o.sprite_id == sprite_utility::bad_sprite_id)
			o.sprite_id = s.create_sprite();

		const auto position = get_position(o);
		const auto size = get_safe_size(o);
		const auto animation = get_random_animation(o);

		s.set_animation(o.sprite_id, animation, {});
		s.set_position(o.sprite_id, position);
		s.set_size(o.sprite_id, size);
	}

	static constexpr auto quad_bucket_limit = 5;

	void level_editor_objects::level_load(const level &l)
	{
		const auto& o = l.objects;
		_objects.next_id = o.next_id;
		_level_limit = { static_cast<float>(l.map_x),
						 static_cast<float>(l.map_y) };

		//setup the quad used for selecting objects
		_quad_selection = object_collision_tree{ rect_float{0.f, 0.f, _level_limit.x, _level_limit.y }, quad_bucket_limit };
		_collision_quads.clear();
		auto sprites = sprite_batch{};
		auto names = std::unordered_map<string, entity_id>{};

		//copy the objects into the editor
		//and set up the sprites and quad data
		std::vector<editor_object_instance> objects{};
		objects.reserve(std::size(o.objects));
		const auto end = std::end(o.objects);
		for (auto iter = std::begin(o.objects); iter != end; ++iter)
		{
			//if the object has a name then try and apply it
			const auto &name = iter->name_id;
			if (!name.empty())
			{
				//try and record the name having been used
				auto[name_iter, name_already_used] = names.try_emplace(name, iter->id);
				std::ignore = name_iter;

				//log error if duplicate names
				if (!name_already_used)
				{
					using namespace std::string_literals;
					const auto msg = "error loading level, name: "s + name
						+ " has already been used by another entity"s;
					LOGERROR(msg);
				}
			}

			//update the quad data and sprites
			auto object = editor_object_instance{ *iter };
			_update_quad_data(object);
			update_object_sprite(object, sprites);
			
			objects.emplace_back(std::move(object));
		}

		using std::swap;
		swap(_objects.objects, objects);
		swap(_sprites, sprites);
		swap(_objects.entity_names, names);

		_brush_type = brush_type::object_selector;
		_held_object = std::nullopt;
		_held_preview = sf::Sprite{};
	}

	level level_editor_objects::level_save(level l) const
	{
		auto& o = l.objects;
		o.next_id = _objects.next_id ;
		o.objects.reserve(std::size(_objects.objects));
		std::copy(std::begin(_objects.objects), std::end(_objects.objects), std::back_inserter(o.objects));
		return l;
	}

	//TODO: replace calls to this with the helper in gui
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

		const auto indent_amount = g.get_item_rect_max().x;

		auto x2 = 0.f;
		for (const auto o : objects)
		{
			const auto new_x2 = x2 + button_size.x;
			if (indent_amount + new_x2 < toolbox_width)
				g.layout_horizontal();
			else
				g.indent();

			auto clicked = false;
			
			string name{};
			if(has_curve(*o, *name_curve))
				name = get_name(*o);
			else
				name = data::get_as_string(o->id);

			g.push_id(o);
			if (const auto ico = get_editor_icon(*o); ico)
				clicked = g.image_button(*ico, button_size);	
			else
				clicked = g.button(name, button_size_no_img);
			g.pop_id();

			g.tooltip(name);

			if (clicked)
				std::invoke(on_click, o);

			x2 = g.get_item_rect_max().x;
		}
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
			_obj_ui.set_selected(bad_entity);
		}

		if (g.toolbar_button("remove") 
			&& _brush_type == brush_type::object_selector
			&& _held_object)
		{
			//erase the selected object
			_remove_object(_held_object->id);

			_held_object.reset();
			_obj_ui.set_selected(bad_entity);
			_held_preview = sf::Sprite{};
		}

		g.main_toolbar_end();

		//TODO:
		//if(*)
		g.window_begin(editor::gui_names::toolbox);

		const auto toolbox_width = g.get_item_rect_max().x;

		if (g.collapsing_header("objects"sv))
		{
			g.checkbox("show objects"sv, _show_objects);
			g.checkbox("allow_intersection"sv, _allow_intersect);

			/*constexpr auto all_str = "all"sv;
			constexpr auto all_index = 0u;*/
			auto on_click_object = [this](const resources::object *o) {
				activate_brush();
				_brush_type = brush_type::object_place;
				_held_object = make_instance(o);

				if (_grid.enabled && _grid.snap
					&& _grid.auto_mode->load())
				{
					const auto grid_size = _grid.size->load();
					const auto obj_size = get_safe_size(*_held_object);
					const auto step = calculate_grid_step_for_size(grid_size, std::max(obj_size.x, obj_size.y));
					_grid.step->store(std::clamp(step, 0, _grid.step_max->load()));
				}
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

		if (g.collapsing_header("properties"sv))
		{
			_obj_ui.object_properties(g);
		}

		g.window_end();

		//NOTE: this is the latest in a frame that we can call this
		// all on_* functions and other non-const functions should 
		// have already been called if appropriate
		_sprites.apply();
	}

	static bool within_level(vector_float pos, vector_float size, vector_float level_size) noexcept
	{
		if (const auto br_corner = pos + size; pos.x < 0.f || pos.y < 0.f
			|| br_corner.x > level_size.x || br_corner.y > level_size.y)
			return false;
		else
			return true;
	}

	//TODO: remove some params, pass in the snapped position in pos, if needed
	//		no need for all the grid settings stuff
	template<typename Object>
	std::variant<sf::Sprite, sf::RectangleShape> make_held_preview(vector_float pos, vector_float level_limit,
		const Object &o, const resources::level_editor_object_settings &s, bool snap, float cell_size)
	{
		const auto size = get_safe_size(o);

		if (!within_level(pos, size, level_limit))
			return std::variant<sf::Sprite, sf::RectangleShape>{};

		const auto obj_pos = snap ? mouse::snap_to_grid(pos, cell_size) : pos;

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
			const auto round_size = vector_int{
				static_cast<int32>(size.x), 
				static_cast<int32>(size.y)
			};

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
			const auto cell_size = calculate_grid_size(*_grid.size, *_grid.step);
			_held_preview = make_held_preview(pos, _level_limit, *_held_object, *_settings, _grid.enabled->load() && _grid.snap->load(), cell_size);
		}break;
		case brush_type::object_selector:
		{
			if (_held_object && _show_objects)
				update_selection_rect(*_held_object, _held_preview);
		}break;
		}
	}

	void level_editor_objects::draw_brush_preview(sf::RenderTarget &t, time_duration, sf::RenderStates s)
	{
		//NOTE: always draw, we're only called when we are the active brush
		// if one of the brush types doesn't use _held_preview, then we'll need to draw conditionally
		std::visit([&t, s](auto &&p) {
			t.draw(p, s);
		}, _held_preview);
	}

	tag_list level_editor_objects::get_tags_at_location(rect_float area) const
	{
		auto tags = tag_list{};

		for(const auto &o : _objects.objects)
		{
			const auto pos = get_position(o);
			const auto size = get_size(o);
			const auto rect = rect_float{ pos, size };

			if (intersects(rect, area))
			{
				const auto obj_tags = get_tags(o);
				tags.reserve(tags.size() + obj_tags.size());
				std::copy(std::begin(obj_tags), std::end(obj_tags),
					std::back_inserter(tags));
			}
		}

		return tags;
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

	void level_editor_objects::on_click(mouse_pos pos)
	{
		assert(_brush_type != brush_type::object_drag);

		if (_brush_type == brush_type::object_selector
			&& _show_objects 
			&& within_level(pos, vector_float{}, _level_limit))
		{
			const auto id = object_at(pos, _quad_selection);
			if (id == bad_entity) // nothing under cursor
				return;

			const auto obj = std::find_if(std::cbegin(_objects.objects), std::cend(_objects.objects), [id](auto &&o) {
				return o.id == id;
			});

			//object was in quadmap but not objectlist
			assert(obj != std::cend(_objects.objects));

			_held_object = *obj;
			_obj_ui.set_selected(_held_object->id);
		}
		else if (_brush_type == brush_type::object_place)
		{
			const auto snapped_pos = snap_to_grid(pos, _grid);

			_try_place_object(snapped_pos, *_held_object);
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

			const auto obj = std::find_if(std::cbegin(_objects.objects), std::cend(_objects.objects), [id](auto &&o) {
				return id == o.id;
			});

			assert(obj != std::cend(_objects.objects));
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

			const auto snapped_pos = snap_to_grid(pos, _grid);

			_try_place_object(snapped_pos, *_held_object);
		}

		//TODO: if object selector_selection rect stretch the rect
	}

	void level_editor_objects::on_drag_end(mouse_pos pos)
	{
		if (_brush_type == brush_type::object_drag)
		{
			const auto snapped_pos = snap_to_grid(pos, _grid);

			if (_try_place_object(snapped_pos, *_held_object))
			{
				const auto obj = std::find_if(std::cbegin(_objects.objects), std::cend(_objects.objects), [id = _held_object->id](auto &&o){
					return id == o.id;
				});

				assert(obj != std::cend(_objects.objects));

				_held_object = *obj;
				update_selection_rect(*_held_object, _held_preview);
			}

			_brush_type = brush_type::object_selector;
		}
	}

	void level_editor_objects::draw(sf::RenderTarget &t, time_duration, sf::RenderStates s)
	{
		if (_show_objects)
		{
			t.draw(_sprites, s);
		}
	}
	
	bool level_editor_objects::_object_valid_location(const rect_float& r, const object_instance& o) const
	{
		const auto id = o.id;

		const auto collision_groups = get_collision_groups(o);
		const auto& current_groups = _collision_quads;

		if (!within_level(position(r), size(r), _level_limit))
			return false;

		if (_allow_intersect)
			return true;

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
					[&r, id](const quad_data<entity_id, rect_float>& other) {
						return other.key != id && intersects(other.rect, r);
				});
		});

		//TODO: a way to ask the terrain system for it's say on this object?
		return !object_collision;
	}

	void level_editor_objects::_remove_object(entity_id id)
	{
		//we assume the the object isn't duplicated in the object list
		const auto obj = std::find_if(std::begin(_objects.objects), std::end(_objects.objects), [id](auto &&o) {
			return o.id == id;
		});

		_sprites.destroy_sprite(obj->sprite_id);
		_quad_selection.remove(obj->id);

		for (auto& [name, tree] : _collision_quads)
		{
			std::ignore = name;
			tree.remove(obj->id);
		}

		_obj_ui.erase(id);
	}

	bool level_editor_objects::_try_place_object(vector_float pos, editor_object_instance o)
	{
		const auto size = get_size(o);

		const auto new_entity = o.id == bad_entity;

		if(new_entity)
			o.id = _objects.next_id;
		
		const auto valid_location = _object_valid_location({ pos, size }, o);

		if (valid_location || _allow_intersect)
		{
			if(new_entity)
				increment(_objects.next_id);

			set_position(o, pos);
			_update_quad_data(o);
			update_object_sprite(o, _sprites);

			const auto end = std::end(_objects.objects);

			const auto obj = std::find_if(std::begin(_objects.objects), end, [id = o.id](auto &&o) {
				return o.id == id;
			});

			if (obj == end)
				_objects.objects.emplace_back(std::move(o));
			else
				*obj = std::move(o);

			//_obj_ui.set_selected(bad_entity);

			return true;
		}

		return false;
	}

	void level_editor_objects::_update_changed_obj(editor_object_instance& o)
	{
		update_object_sprite(o, _sprites);
		_update_quad_data(o);
		return;
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
			auto[group, found] = _collision_quads.try_emplace(col_group_id,
				rect_float{ {0.f, 0.f}, _level_limit }, quad_bucket_limit);

			std::ignore = found;
			group->second.insert(rect, o.id);
		}
	}

	level_editor_objects::editor_object_instance::editor_object_instance(const object_instance &o)
		: object_instance{o}
	{}
}