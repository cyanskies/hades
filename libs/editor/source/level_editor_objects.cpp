#include "hades/level_editor_objects.hpp"

#include "hades/animation.hpp"
#include "hades/background.hpp"
#include "hades/core_curves.hpp"
#include "hades/gui.hpp"
#include "hades/level_editor.hpp"
#include "hades/level_editor_grid.hpp"
#include "hades/mouse_input.hpp"
#include "hades/parser.hpp"

namespace obj_fn = hades::resources::object_functions;
using namespace std::string_view_literals;
constexpr auto level_editor_object_resource_name = "level-editor-object-settings"sv;
static auto object_settings_id = hades::unique_id::zero;
[[deprecated]] constexpr auto reserved_object_names = {
	"world" // reserved for the world object
};

namespace hades::resources
{
	void level_editor_object_settings::serialise(const data::data_manager& d, data::writer& w) const
	{
		// object groups
		if (!empty(groups))
		{
			w.start_map("object-groups"sv);

			for (const auto& g : groups)
			{
				const auto& list = std::get<1>(g);
				if (empty(list))
					continue;

				w.write(std::get<0>(g));
				w.start_sequence();
				for (const auto& obj : list)
					w.write(d.get_as_string(obj.id()));
				w.end_sequence();
			}

			w.end_map();
		}

		//object colour
		if (object_colour != default_colour)
		{
			const auto hades_colour = colour{ object_colour.r, object_colour.g, object_colour.b, object_colour.a };
			w.write("object-colour"sv, to_string(hades_colour));
		}
	}

	static inline void parse_level_editor_object_resource(unique_id mod, const data::parser_node &node, data::data_manager &d)
	{
		//level-editor-object-settings:
		//    object-groups:
		//        group-name: [elms, elms]
		//		  object-colour: red

		auto settings = d.find_or_create<level_editor_object_settings>(object_settings_id, mod, level_editor_object_resource_name);

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
				auto &group_content = std::get<std::vector<resource_link<object>>>(group);

				group_content = g->merge_sequence(std::move(group_content), [&d, mod](const std::string_view str_id) {
					const auto id = d.get_uid(str_id);
					return d.make_resource_link<object>(id, object_settings_id);
				});
			}
		}//!object_groups

		// object colour
		const auto col_node = node.get_child("object-colour"sv);
		if (col_node)
		{
			const auto col = col_node->to_scalar<colour>();
			settings->object_colour = sf::Color{ col.r, col.g, col.b, col.a };
		}
	}
}

namespace hades
{
	void register_level_editor_object_resources(data::data_manager &d)
	{
		register_objects(d);

		d.register_resource_type(level_editor_object_resource_name, resources::parse_level_editor_object_resource);

		//create default settings object
		object_settings_id = d.get_uid(level_editor_object_resource_name);
		std::ignore = d.find_or_create<resources::level_editor_object_settings>(object_settings_id, {}, level_editor_object_resource_name);
	}

	level_editor_objects_impl::level_editor_objects_impl()
		: _obj_ui{ &_objects, [this](editor_object_instance* o) {
				assert(o);
				return _update_changed_obj(*o);
			} ,
			[this](entity_id id) {
				_remove_object_callback(id);
				return;
			} 
		}
	{
		if (object_settings_id != unique_id::zero)
			_settings = data::get<resources::level_editor_object_settings>(object_settings_id);

		auto objects = data::get_all_ids_for_type("objects"sv);
		_object_types.reserve(size(objects));
		for (const auto& o : objects)
		{
			const auto obj = data::get<resources::object>(o, data::no_load);
			if (!obj->loaded)
				data::get<resources::object>(o);
			_object_types.emplace_back(obj);
		}
	}

	//TODO: make into member function
	static vector2_float get_safe_size(const object_instance &o)
	{
		const auto size = get_size(o);
		if (size.x == 0.f || size.y == 0.f)
		{
			const auto tile_scale = console::get_int(cvars::editor_level_force_whole_tiles,
				cvars::default_value::editor_level_force_whole_tiles);
			if (tile_scale == 0)
				return { 8.f, 8.f };

			const auto cell_size = static_cast<float>(resources::get_tile_size() * tile_scale->load());
			return { cell_size, cell_size };
		}
		else
			return size;
	}

	static constexpr auto grid_size = 64.f;

	void level_editor_objects_impl::level_load(const level &l)
	{
		const auto& o = l.objects;
		_objects.next_id = o.next_id;
		_level_limit = { static_cast<float>(l.map_x),
						 static_cast<float>(l.map_y) };

		//setup the quad used for selecting objects
		_quad_selection = object_collision_grid{ rect_float{0.f, 0.f, _level_limit.x, _level_limit.y }, grid_size };
		_collision_quads.clear();
		auto sprites = sprite_batch{};
		auto names = unordered_map_string<entity_id>{};

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
			if (has_curve(object, get_position_curve_id()) &&
				has_curve(object, get_size_curve_id()))
			{
				_update_quad_data(object);
				create_update_object_sprite(object, sprites);
			}

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

	level level_editor_objects_impl::level_save(level l) const
	{
		auto& o = l.objects;
		o.next_id = _objects.next_id ;
		o.objects.insert(std::end(o.objects), std::begin(_objects.objects), std::end(_objects.objects));
		return l;
	}

	void level_editor_objects_impl::level_resize(vector2_int size, vector2_int offset)
	{
		const auto offset_f = static_cast<vector2_float>(offset);

		_level_limit = static_cast<vector2_float>(size);
		const auto new_world_limit = rect_float{
			{}, _level_limit
		};

		_quad_selection = object_collision_grid{ new_world_limit, grid_size };
		_collision_quads.clear();

		auto removal_list = std::vector<entity_id>{};
		removal_list.reserve(std::size(_objects.objects));

		const auto position_id = get_position_curve_id();
		const auto position_c = get_position_curve();
		const auto size_id = get_size_curve_id();
		const auto size_c = get_size_curve();
		for (auto& o : _objects.objects)
		{
			const auto id = o.id;
			if (has_curve(o, position_id) &&
				has_curve(o, size_id))
			{
				const auto old_pos = get_curve_value<resources::curve_types::vec2_float>(o, *position_c);
				const auto pos = old_pos + offset_f;
				set_curve(o, *position_c, pos);

				const auto siz = get_curve_value<resources::curve_types::vec2_float>(o, *size_c);
				
				if (is_within(rect_t{ pos, siz }, new_world_limit))
				{
					_update_changed_obj(o);
					on_object_place(o);
				}
				else
					removal_list.emplace_back(id);
			}
		}

		std::ranges::for_each(removal_list, [this](auto&& id) {
			_remove_object(id);
			return;
			});

		return;
	}

	template<typename Func, typename ObjectPtr>
	static void add_object_buttons(gui &g, const std::vector<ObjectPtr> &objects, Func on_click)
	{
		static_assert(std::is_invocable_v<Func, const resources::object*>);

		constexpr auto button_size = vector2_float{ 25.f, 25.f };
		constexpr auto button_scale_diff = vector2_float{ 8.f, 6.f };
		constexpr auto button_size_no_img = button_size + button_scale_diff;

		const auto name_curve = get_name_curve();

		for (const auto o : objects)
		{
			auto clicked = false;

			auto name = string{};
			using obj_fn::has_curve;
			if (has_curve(*o, *name_curve))
				name = get_name(*o);
			else
				name = data::get_as_string(o->id);

			g.push_id(&*o);
			if (const auto ico = get_editor_icon(*o); ico)
			{
				g.same_line_wrapping(g.calculate_button_size({}, button_size).x);
				clicked = g.image_button("###obj_button"sv, *ico, button_size);
			}
			else
			{
				g.same_line_wrapping(g.calculate_button_size(name, button_size_no_img).x);
				clicked = g.button(name, button_size_no_img);
			}

			const auto s = g.get_item_rect_size();

			g.pop_id();

			g.tooltip(name);
			// for debugging the button size
			//g.tooltip(name + " x: " + to_string(s.x) + ", y: " + to_string(s.y));

			if (clicked)
				std::invoke(on_click, &*o);
		}

		return;
	}

	void level_editor_objects_impl::gui_update(gui &g, editor_windows&)
	{
		using namespace std::string_literals;
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

		if (g.toolbar_button("remove"sv) 
			&& _brush_type == brush_type::object_selector
			&& _held_object)
		{
			//erase the selected object
			_remove_object(_held_object->id);
		}

		g.main_toolbar_end();

		if (g.window_begin(editor::gui_names::toolbox))
		{
			if (g.collapsing_header("objects"sv))
			{
				g.checkbox("show objects"sv, _show_objects);
				g.checkbox("allow_intersection"sv, _allow_intersect);

				const auto player_preview = _object_owner == unique_zero ? "none"s
					: data::get_as_string(_object_owner);

				if (g.combo_begin("player"sv, player_preview))
				{
					const auto players = get_players();
					for (auto& [p, o] : players)
					{
						if (g.selectable(data::get_as_string(p), _object_owner == p))
						{
							_object_owner = p;
							if (_held_object.has_value())
								set_curve(*_held_object, *get_player_owner_curve(), _object_owner);
						}
					}

					if (g.selectable("none"sv, _object_owner == unique_zero))
						_object_owner = unique_zero;

					g.combo_end();
				}

				auto on_click_object = [this](const resources::object* o) {
					activate_brush();
					_brush_type = brush_type::object_place;
					_held_object = make_instance(o);
					set_curve(*_held_object, *get_player_owner_curve(), _object_owner);

					if (_grid.auto_mode)
						set_grid_settings_for_object_type(_grid, *_held_object);
				};

				g.indent();

				if (g.collapsing_header("all"sv))
					add_object_buttons(g, _object_types, on_click_object);

				for (const auto& group : _settings->groups)
				{
					g.indent();

					if (g.collapsing_header(std::get<string>(group)))
						add_object_buttons(g, std::get<std::vector<resources::resource_link<resources::object>>>(group), on_click_object);
				}
			}

			if (g.collapsing_header("object list"sv))
			{
				_obj_ui.show_object_list_buttons(g);
				_obj_ui.object_list_gui(g);
			}

			if (g.collapsing_header("properties"sv))
				_obj_ui.object_properties(g);
		}
		g.window_end();

		//NOTE: this is the latest in a frame that we can call this
		// all on_* functions and other non-const functions should 
		// have already been called if appropriate
		_sprites.apply();
	}

	constexpr bool within_level(vector2_float pos, vector2_float size, vector2_float level_size) noexcept
	{
		return is_within(pos, { {0.f, 0.f}, level_size });
	}

	//TODO: remove some params, pass in the snapped position in pos, if needed
	//		no need for all the grid settings stuff
	template<typename Object>
	std::variant<sf::Sprite, sf::RectangleShape> make_held_preview(vector2_float pos, vector2_float level_limit,
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
			const auto bounds = sprite.getLocalBounds();
			sprite.setScale({ size.x / bounds.width, size.y / bounds.height });
			sprite.setPosition({ obj_pos.x, obj_pos.y });
			sf::Color col = sprite.getColor();

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

	void level_editor_objects_impl::make_brush_preview(time_duration, mouse_pos pos)
	{
		switch (_brush_type)
		{
		case brush_type::object_place:
			[[fallthrough]];
		case brush_type::object_drag:
		{
			assert(_held_object);
			const auto cell_size = calculate_grid_size(*_grid.step);
			_held_preview = make_held_preview(pos, _level_limit, *_held_object, *_settings, _grid.enabled->load() && _grid.snap->load(), cell_size);
		}break;
		case brush_type::object_selector:
		{
			if (_held_object && _show_objects)
				update_selection_rect(*_held_object, _held_preview);
		}break;
		}
	}

	void level_editor_objects_impl::draw_brush_preview(sf::RenderTarget &t, time_duration, sf::RenderStates s)
	{
		//NOTE: always draw, we're only called when we are the active brush
		// if one of the brush types doesn't use _held_preview, then we'll need to draw conditionally
		std::visit([&t, s](auto &&p) {
			t.draw(p, s);
		}, _held_preview);
	}

	tag_list level_editor_objects_impl::get_object_tags_at_location(rect_float area) const
	{
		auto tags = tag_list{};

		const auto pos_id = get_position_curve();
		const auto siz_id = get_size_curve();

		for(const auto &o : _objects.objects)
		{
			const auto pos = get_curve_value<curve_types::vec2_float>(o, *pos_id);
			const auto siz = get_curve_value<curve_types::vec2_float>(o, *siz_id);
			const auto rect = rect_float{ pos, siz };

			if (intersects(rect, area))
			{
				assert(o.obj_type);
				const auto& obj_tags = obj_fn::get_tags(*o.obj_type);
				tags.insert(std::end(tags), std::begin(obj_tags), std::end(obj_tags));
			}
		}

		return tags;
	}

	static entity_id object_at(level_editor_objects_impl::mouse_pos pos, 
		const level_editor_objects_impl::object_collision_grid &quads,
		const std::vector<level_editor_objects_impl::editor_object_instance>& objects)
	{
		const auto target = rect_float{ {pos.x - .5f, pos.y - .5f}, {.5f, .5f} };
		const auto rects = quads.find(target);

		const auto pos_id = get_position_curve();
		const auto siz_id = get_size_curve();

		for (const auto o : rects)
		{
			const auto obj = std::ranges::find(objects, o, &level_editor_objects_impl::editor_object_instance::id);

			assert(obj != end(objects));
			const auto p = get_curve_value<curve_types::vec2_float>(*obj, *pos_id);
			const auto s = get_curve_value<curve_types::vec2_float>(*obj, *siz_id);
			if (intersects({ p, s }, target))
				return o;
		}

		return bad_entity;
	}

	void level_editor_objects_impl::on_click(mouse_pos pos)
	{
		assert(_brush_type != brush_type::object_drag);

		if (_brush_type == brush_type::object_selector
			&& _show_objects 
			&& within_level(pos, vector2_float{}, _level_limit))
		{
			const auto id = object_at(pos, _quad_selection.value(), _objects.objects);
			if (id == bad_entity) // nothing under cursor
				return;

			const auto obj = std::ranges::find(_objects.objects, id, &editor_object_instance::id);

			//object was in quadmap but not objectlist
			assert(obj != std::cend(_objects.objects));

			_held_object = *obj;
			_obj_ui.set_selected(_held_object->id);
		}
		else if (_brush_type == brush_type::object_place)
		{
			if (!is_within(pos, rect_float{ {0.f, 0.f}, _level_limit }))
				return;

			const auto snapped_pos = snap_to_grid(pos, _grid);

			_try_place_object(snapped_pos, *_held_object);
		}
	}

	void level_editor_objects_impl::on_drag_start(mouse_pos pos)
	{
		if (!within_level(pos, vector2_float{}, _level_limit))
			return;

		if (_brush_type == brush_type::object_selector
			&& _show_objects)
		{
			const auto id = object_at(pos, _quad_selection.value(), _objects.objects);

			if (id == bad_entity)
				return;

			const auto obj = std::ranges::find(_objects.objects, id, &editor_object_instance::id);

			assert(obj != std::cend(_objects.objects));
			_held_object = *obj;

			_brush_type = brush_type::object_drag;
		}
	}

	void level_editor_objects_impl::on_drag(mouse_pos pos)
	{
		if (!_show_objects)
			return;

		if (_brush_type == brush_type::object_place)
		{
			assert(_held_object);

			const auto obj_size = get_size(*_held_object);
			pos.x = std::clamp(pos.x, 0.f, _level_limit.x - obj_size.x);
			pos.y = std::clamp(pos.y, 0.f, _level_limit.y - obj_size.x);

			const auto snapped_pos = snap_to_grid(pos, _grid);

			_try_place_object(snapped_pos, *_held_object);
		}

		//TODO: if object selector_selection rect stretch the rect
	}

	void level_editor_objects_impl::on_drag_end(mouse_pos pos)
	{
		if (_brush_type == brush_type::object_drag)
		{
			const auto snapped_pos = snap_to_grid(pos, _grid);

			const auto id = _held_object->id;
			if (_try_place_object(snapped_pos, *_held_object))
			{
				const auto obj = std::find_if(std::cbegin(_objects.objects), std::cend(_objects.objects), [id](auto &&o){
					return id == o.id;
				});

				assert(obj != std::cend(_objects.objects));

				_held_object = *obj;
				update_selection_rect(*_held_object, _held_preview);
				_obj_ui.set_selected(id);
			}

			// todo: if try place returned false, selection is mucked up

			_brush_type = brush_type::object_selector;
		}
	}

	void level_editor_objects_impl::draw(sf::RenderTarget &t, time_duration, sf::RenderStates s)
	{
		if (_show_objects)
		{
			t.draw(_sprites, s);
		}
	}

	const std::vector<level_editor_objects_impl::editor_object_instance>& level_editor_objects_impl::get_objects() const noexcept
	{
		return _objects.objects;
	}

	const level_editor_objects_impl::object_collision_grid& level_editor_objects_impl::get_quadmap() const noexcept
	{
		return _quad_selection.value();
	}

	const level_editor_objects_impl::collision_layer_map& level_editor_objects_impl::get_collision_layers() const noexcept
	{
		return _collision_quads;
	}

	world_vector_t level_editor_objects_impl::get_level_size() const noexcept
	{
		return _level_limit;
	}

	std::optional<world_vector_t> level_editor_objects_impl::closest_valid_position(world_rect_t rect, const object_instance& o) const
	{
		if (_object_valid_location(rect, o))
			return position(rect);

		return std::nullopt;
	}


	void level_editor_objects_impl::set_grid_settings_for_object_type(grid_vars& g, object_instance&)
	{
		const auto obj_size = get_safe_size(*_held_object);
		const auto step = calculate_grid_step_for_size(std::max(obj_size.x, obj_size.y));
		g.step->store(std::clamp(step, 0, g.step_max->load()));
	}

	void level_editor_objects_impl::create_update_object_sprite(editor_object_instance& o, sprite_batch& s)
	{
		if (o.sprite_id == sprite_utility::bad_sprite_id)
			o.sprite_id = s.create_sprite();

		const auto position = get_position(o);
		const auto size = get_safe_size(o);
		const auto animation = get_random_animation(o);

		s.set_animation(o.sprite_id, animation, {});
		s.set_position(o.sprite_id, position);
		s.set_size(o.sprite_id, size);
		return;
	}
	
	bool level_editor_objects_impl::_object_valid_location(const rect_float& r, const object_instance& o) const
	{
		const auto id = o.id;

		const auto collision_group = get_collision_groups(o);
		const auto& current_groups = get_collision_layers();

		if (!within_level(position(r), size(r), get_level_size()))
			return false;

		//for each group that this object is a member of
		//check each neaby rect for a collision
		//return true if any such collision would occur
		const auto group_quadtree = current_groups.find(collision_group);
		if (group_quadtree == std::end(current_groups))
			return false;

		const auto local_rects = group_quadtree->second.find(r);
		const auto& objects = get_objects();

		const auto pos_id = get_position_curve();
		const auto siz_id = get_size_curve();

		const auto object_collision = std::any_of(std::begin(local_rects), std::end(local_rects),
			[&](auto&& other) {
				if (other == id)
					return false;

				auto obj = std::ranges::find(objects, other, &editor_object_instance::id);
				assert(obj != end(objects));

				const auto pos = get_curve_value<curve_types::vec2_float>(*obj, *pos_id);
				const auto siz = get_curve_value<curve_types::vec2_float>(*obj, *siz_id);

				return intersects({ pos, siz }, r);
			});

		//TODO: a way to ask the terrain system for it's say on this object?
		return !object_collision;
	}


	void level_editor_objects_impl::_remove_object_callback(entity_id id)
	{
		//we assume the the object isn't duplicated in the object list
		const auto obj = std::find_if(std::begin(_objects.objects), std::end(_objects.objects), [id](auto&& o) {
			return o.id == id;
			});

		_sprites.destroy_sprite(obj->sprite_id);
		_quad_selection.value().remove(obj->id);

		for (auto& [name, tree] : _collision_quads)
		{
			std::ignore = name;
			tree.remove(obj->id);
		}

		_held_object.reset();
		_held_preview = sf::Sprite{};

		return;
	}

	void level_editor_objects_impl::_remove_object(entity_id id)
	{
		_obj_ui.erase(id);
		return;
	}

	bool level_editor_objects_impl::_try_place_object(vector2_float pos, editor_object_instance o)
	{
		const auto size = get_size(o);

		const auto new_entity = o.id == bad_entity;

		if(new_entity)
			o.id = _objects.next_id;
		
		const auto valid_location = closest_valid_position({ pos, size }, o);

		if (valid_location || _allow_intersect)
		{
			if(new_entity)
				increment(_objects.next_id);

			if(valid_location)
				set_position(o, valid_location.value());
			else
				set_position(o, pos);

			_update_changed_obj(o);
			on_object_place(o);

			const auto end = std::end(_objects.objects);

			const auto obj = std::find_if(std::begin(_objects.objects), end, [id = o.id](auto &&o) {
				return o.id == id;
			});

			if (obj == end)
				_objects.objects.emplace_back(std::move(o));
			else
				*obj = std::move(o);

			return true;
		}

		return false;
	}

	void level_editor_objects_impl::_update_changed_obj(editor_object_instance& o)
	{
		create_update_object_sprite(o, _sprites);
		_update_quad_data(o);
		return;
	}

	void level_editor_objects_impl::_update_quad_data(const object_instance &o)
	{
		//update selection quad
		const auto position = get_position(o);

		{
			const auto select_size = get_safe_size(o);
			_quad_selection.value().update(o.id, {position, select_size});
		}
		
		//update collision quad
		const auto collision_group = get_collision_groups(o);
		const auto size = get_size(o);
		const auto rect = rect_float{ position, size };

		//remove this object from all the quadtrees
		//this is needed if the object is no longer 
		//part of a specific collision group
		for (auto &group : _collision_quads)
			group.second.remove(o.id);

		//emplace creates the entry if needed, otherwise returns
			//the existing one, we don't care which
		using rect_type = object_collision_grid::rect_type;
		auto [group, found] = _collision_quads.try_emplace(collision_group,
			rect_type{ { 0.f, 0.f }, _level_limit }, grid_size);

		std::ignore = found;
		group->second.insert(o.id, rect);
		return;
	}

	level_editor_objects_impl::editor_object_instance::editor_object_instance(const object_instance &o)
		: object_instance{o}
	{}
}
