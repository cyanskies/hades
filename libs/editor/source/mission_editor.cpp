#include "hades/mission_editor.hpp"

#include "hades/level_editor.hpp"
#include "hades/objects.hpp"
#include "hades/parser.hpp"

namespace hades
{
	void create_mission_editor_console_variables()
	{
		console::create_property(cvars::editor_mission_ext, cvars::default_value::editor_mission_ext);
		console::create_property(cvars::editor_lock_player_object_type, cvars::default_value::editor_lock_player_object_type);
		
		create_level_editor_console_vars();
		return;
	}

	void register_mission_editor_resources(data::data_manager& d)
	{
		register_level_editor_resources(d);
		register_objects(d);
		return;
	}

	void mission_editor_t::init()
	{
		_obj_ui.emplace(&_objects,
			[&](object_instance& o) {
				on_add_object(o);
				return;
			});
		
		if (_mission_src == path{})
		{
			const auto mis = new_mission();

			//create an empty mission
			_mission_desc = mis.description;
			_mission_name = mis.name;
		}
		else
		{
			//load from path
			_load(_mission_src);
		}

		reinit();
		return;
	}

	bool mission_editor_t::handle_event(const event& e)
	{
		_gui.activate_context();
		return _gui.handle_event(e);
	}

    void mission_editor_t::update(time_duration dt, const sf::RenderTarget&, input_system::action_set)
	{
		_editor_time += dt;
		_update_gui(dt);
	}

	void mission_editor_t::draw(sf::RenderTarget &t, time_duration)
	{
		t.setView(_gui_view);
		t.draw(_backdrop);
		_gui.activate_context();
		t.resetGLStates();
		t.draw(_gui);
	}

	void mission_editor_t::reinit()
	{
		const auto width = console::get_int(cvars::video_width, cvars::default_value::video_width);
		const auto height = console::get_int(cvars::video_height, cvars::default_value::video_height);

		const auto fwidth = static_cast<float>(width->load());
		const auto fheight = static_cast<float>(height->load());

		_gui_view.reset({ {}, { fwidth, fheight } });
		_gui.activate_context();
		_gui.set_display_size({ fwidth, fheight });

		_backdrop.setPosition({0.f, 0.f});
		_backdrop.setSize({ fwidth, fheight });
		const auto background_colour = sf::Color{ 200u, 200u, 200u, 255u };
		_backdrop.setFillColor(background_colour);
	}

	mission_editor_t::get_players_return_type mission_editor_t::get_players() const
	{
		auto out = detail::level_editor_impl::get_players_return_type{};
		out.reserve(size(_players));
		for (const auto& [name, obj_id] : _players)
			out.emplace_back(name, _obj_ui.value().get_obj(obj_id));
		return out;
	}

	mission mission_editor_t::new_mission()
	{
		//create an empty mission
		auto mis = mission{};
		mis.description = "An empty mission";
		mis.name = "Empty mission";

		//no starting ents or levels

		return mis;
	}

	unique_id mission_editor_t::default_new_player_object() const noexcept
	{
		return unique_zero;
	}

	void mission_editor_t::_gui_menu_bar()
	{
		if (_gui.menu_begin("mission editor"))
		{
			if (_gui.menu_item("new...", false))
			{
				;//TODO: error model for unimp
			}

			if (_gui.menu_item("load..."))
			{
				_load_window.open = true;
				_load_window.path = _mission_src.generic_string();
			}
			if(_gui.menu_item("save", !std::empty(_mission_src)))
				_save({_mission_src});
			if (_gui.menu_item("save as..."))
			{
				_save_window.open = true;
				_save_window.path = _mission_src.generic_string();
			}
			if (_gui.menu_item("exit"))
				kill();
			_gui.menu_end();
		}

		if (_gui.menu_begin("windows"))
		{
			_gui.menu_toggle_item("show level window", _level_window_state.open);
			_gui.menu_toggle_item("show object window", _obj_w);
			_gui.menu_toggle_item("show object properties windows", _obj_prop_w);
			_gui.menu_toggle_item("show players window", _player_window_state.open);
			_gui.menu_end();
		}

		return;
	}

	void mission_editor_t::_gui_players_window()
	{
		if (_gui.button("add..."))
		{
			_player_dialog.open = true;
			const auto obj = default_new_player_object();
			if (obj == unique_zero)
				_player_dialog.player_ent = {};
			else
				_player_dialog.player_ent = data::get_as_string(obj);
		}

		if (!std::empty(_players))
		{
			_gui.layout_horizontal();
			if (_gui.button("edit...") && !std::empty(_players))
				_player_window_state.edit_open = true;
			_gui.layout_horizontal();
			if (_gui.button("remove"))
			{
                auto iter = std::next(std::begin(_players), signed_cast(_player_window_state.selected));

				if(iter->object != bad_entity)
					_obj_ui.value().erase(iter->object);

				iter = _players.erase(iter);

				if (_player_window_state.selected != 0
					&& _player_window_state.selected == std::size(_players))
					--_player_window_state.selected;

				_player_window_state.edit_open = false;
			}

			if (_gui.button("move up")
				&& _player_window_state.selected > std::size_t{})
			{
                const auto iter = std::next(std::begin(_players), signed_cast(_player_window_state.selected));
				std::iter_swap(iter, std::prev(iter));
				--_player_window_state.selected;
			}
			_gui.layout_horizontal();
			if (_gui.button("move down")
				&& (_player_window_state.selected + 1) < std::size(_players))
			{
                const auto iter = std::next(std::begin(_players), signed_cast(_player_window_state.selected));
				std::iter_swap(iter, std::next(iter));
				++_player_window_state.selected;
			}

			if (!std::empty(_players) && _players[_player_window_state.selected].object != bad_entity)
			{
				_gui.layout_horizontal();
				if (_gui.button("select object"))
				{
					_obj_ui.value().set_selected(_players[_player_window_state.selected].object);
				}
			}
		}

		//list
		if (_gui.listbox("##players_list", _player_window_state.selected, _players, [](const player& p)->string {
			return to_string(p.id);
			}))
		{
			_player_dialog = player_dialog_t{};
			_player_window_state.edit_open = false;
		}

		//new player dialog
		if (_player_dialog.open)
		{
			if (_gui.window_begin("new player", _player_dialog.open))
			{
				constexpr auto modal_name = "player creation error";

				if (_gui.button("create"))
				{
					[&, this]() {
						if (std::empty(_player_dialog.name))
						{
							_gui.open_modal(modal_name);
							_player_dialog.error = player_dialog_t::error_type::name_missing;
							return;
						}

						const auto id = data::make_uid(_player_dialog.name);

						if (std::any_of(std::begin(_players), std::end(_players), [id](auto&& p) {
							return id == p.id;
							}))
						{
							_gui.open_modal(modal_name);
							_player_dialog.error = player_dialog_t::error_type::name_used;
							return;
						}

						auto& p = _players.emplace_back();
						p.id = id;
						
						if (_player_dialog.create_ent)
						{
							const auto object_id = data::get_uid(_player_dialog.player_ent);

							try
							{
								const auto obj = data::get<resources::object>(object_id);
								auto o = make_instance(obj);
								on_add_object(o);
								p.object = _obj_ui.value().add(std::move(o));
							}
							catch (const data::resource_null&)
							{
								_gui.open_modal(modal_name);
								_player_dialog.error = player_dialog_t::error_type::null_entity;
								return;
							}
							catch (const data::resource_wrong_type&)
							{
								_gui.open_modal(modal_name);
								_player_dialog.error = player_dialog_t::error_type::entity_wrong_type;
								return;
							}
						}

						_player_dialog = player_dialog_t{};

						return;
					}();
				}

				_gui.input("name", _player_dialog.name);

				constexpr auto create_chkbox = "create entity";
				constexpr auto player_input = "player object";
				if (_player_lock_object_type->load())
				{
					auto always_true = true;
					const auto player_ent = default_new_player_object();
					auto player_str = data::get_as_string(player_ent);
					_gui.checkbox(create_chkbox, always_true);
					_gui.input(player_input, player_str, gui::input_text_flags::readonly);
				}
				else
				{
					_gui.checkbox(create_chkbox, _player_dialog.create_ent);
					_gui.input(player_input, _player_dialog.player_ent);
				}

				//error dialog
				if (_gui.modal_begin(modal_name))
				{
					_gui.text("Unable to create a new player with these settings");

					switch (_player_dialog.error)
					{
					case player_dialog_t::error_type::name_missing:
						_gui.text("players require a name");
						break;
					case player_dialog_t::error_type::name_used:
						_gui.text("a player already has this name");
						break;
					case player_dialog_t::error_type::entity_wrong_type:
						_gui.text(_player_dialog.player_ent + " is not an object type");
						break;
					case player_dialog_t::error_type::null_entity:
						_gui.text(_player_dialog.player_ent + " is not a resource name");
						break;
					default:
						_gui.text("unknown error");
					}

					if (_gui.button("ok"))
						_gui.close_current_modal();

					_gui.modal_end();
				}
			}
			_gui.window_end();

			if (!_player_dialog.open)
				_player_dialog = player_dialog_t{};
		}

		if (_player_window_state.edit_open)
		{
			//TODO: add object resource template to player edit
			if (_gui.window_begin("edit", _player_window_state.edit_open))
			{
				if (_gui.button("done"))
					_player_window_state.edit_open = false;
				auto& p = _players[_player_window_state.selected];
				if (p.object == bad_entity && _gui.button("make entity"))
				{
					auto o = object_instance{};
					p.object = _obj_ui.value().add(std::move(o));
				}
			}
			_gui.window_end();
		}
	}

	static string level_info_to_string(const mission_editor_t::level_info& l)
	{
		if (l.path)
		{
			const auto name = data::get_as_string(l.name);
			return name + "(" + *l.path + ")";
		}
		else //TODO: asterisk to show 'dirty' levels
			return data::get_as_string(l.name);
	}

	void mission_editor_t::_gui_level_window()
	{
		const auto add_level = [](level_window_state_t& s)
		{
			s.add_level_open = true;
			s.add_level_external = false;
			s.rename = false;
			s.rename_index = -1;
			s.new_level_name = string{};
			s.new_level_path = string{};
		};

		const auto rename_level = [](level_window_state_t& s, const std::vector<level_info>& l)
		{
			assert(s.selected < std::size(l));

			const auto& t = l[s.selected];

			s.add_level_open = true;
			s.add_level_external = t.path.has_value();
            s.rename_index = signed_cast(s.selected);
			s.rename = true;
			s.new_level_name = to_string(t.name);
			s.new_level_path = t.path.value_or(string{});
		};

		auto& s = _level_window_state;
		
		//====level window====

		if (_gui.button("add..."))
			add_level(s);

		_gui.layout_horizontal();
		if (_gui.button("remove") && !std::empty(_levels))
		{
			const auto last = std::size(_levels) - 1u;
			if (s.rename)
			{
				if (s.selected == integer_cast<std::size_t>(s.rename_index))
					s.add_level_open = false;
				else if (signed_cast(s.selected) < s.rename_index)
					--s.rename_index;
			}

			if (s.selected == 0u)
			{
				_levels.erase(std::begin(_levels));
			}
			else if (s.selected == last)
			{
				_levels.pop_back();
				--s.selected;
			}
			else
			{
				auto it = std::begin(_levels);
				std::advance(it, s.selected);
				_levels.erase(it);
				--s.selected;
			}
		}

		_gui.button("move up");
		_gui.layout_horizontal();
		_gui.button("move down");

		if (_gui.button("rename"))
			rename_level(s, _levels);

		_gui.layout_horizontal();
		if (_gui.button("edit...") && !std::empty(_levels))
		{
			assert(s.selected < std::size(_levels));
			auto& l = _levels[s.selected];
			if(!l.path.has_value())
				make_editor_state(l);
		}

		_gui.listbox("##levels", s.selected, _levels, level_info_to_string);

		if (s.add_level_open)
		{
			if (_gui.window_begin("add level"))
				_gui_add_level_window();
			_gui.window_end();
		}

		return;
	}

	void mission_editor_t::_gui_add_level_window()
	{
		constexpr auto error_modal = "error##add_level";

		auto& s = _level_window_state;

		_gui.input("name", s.new_level_name);
		_gui.checkbox("external level", s.add_level_external);
		auto input_flags = gui::input_text_flags::readonly;
		if (s.add_level_external)
			input_flags = gui::input_text_flags::none;
		else
			s.new_level_path = string{};
		_gui.input("level path", s.new_level_path, input_flags);

		constexpr auto create = "create##ok_button";
		constexpr auto update = "update##ok_button";

		if (_gui.button(s.rename ? update : create))
		{
			using e = level_window_state_t::errors;

			const auto id = data::make_uid(s.new_level_name);
			const auto name_used = [id, index = s.rename_index](const std::vector<level_info>& l) {
				if (std::empty(l))
					return false;
				
				//index == -1 if this window is opened to create, rather than rename
				if (index < 0)
				{
					return std::any_of(std::begin(l), std::end(l), [id](const level_info& l) {
						return id == l.name;
					});
				}
				else
				{
					assert(index < signed_cast(std::size(l)));
					const auto end = std::size(l);
					for (auto i = std::size_t{}; i < end; ++i)
					{
						//ignore our own name
						if (i != integer_cast<std::size_t>(index) && l[i].name == id)
							return true;
					}
				}

				return false;
			};

			if (s.new_level_name.empty())
			{
				s.current_error = e::name_empty;
				_gui.open_modal(error_modal);
			}
			else if (name_used(_levels))
			{
				s.current_error = e::name_taken;
				_gui.open_modal(error_modal);
			}
			else if (false/*TODO: check valid path*/)
			{
				s.current_error = e::path_not_found;
				_gui.open_modal(error_modal);
			}
			else
			{
				s.add_level_open = false;
				if (s.rename && !std::empty(_levels))
				{
					//rename
					auto& l =_levels[integer_cast<std::size_t>(s.rename_index)];
					l.name = id;
					if (s.add_level_external)
					{
						l.path = s.new_level_path;
                        l.level_data = {}; //erase level if it was previously internal
					}
					else
						l.path.reset();
				}
				else
				{
					//create
                    auto l = level_info{ id, {}, {} };
					if (s.add_level_external)
						l.path = s.new_level_path;
					else
					{
                        l.level_data = make_new_level();
						_gui.activate_context();
                        l.level_data.name = s.new_level_name;
					}
					_levels.emplace_back(std::move(l));
				}
			}
		}

		_gui.layout_horizontal();
		if (_gui.button("cancel"))
			s.add_level_open = false;

		//===level error====
		if (_gui.modal_begin(error_modal))
		{
			using errors = level_window_state_t::errors;

			if (s.current_error == errors::name_empty)
				_gui.text("a name must be provided");
			else if (s.current_error == errors::name_taken)
				_gui.text("the name provided is already used for a level in this mission");
			else if (s.current_error == errors::path_not_found)
				_gui.text("bad path");
			else
			{
				LOGERROR("invalid 'current_error' in mission editor: add_level");
			}

			if (_gui.button("ok"))
				_gui.close_current_modal();

			_gui.modal_end();
		}
	}

	void mission_editor_t::_gui_object_window()
	{
		_obj_ui.value().show_object_list_buttons(_gui);
		_obj_ui.value().object_list_gui(_gui);

		return;
	}

	void mission_editor_t::_update_gui(time_duration dt)
	{
		_gui.activate_context();
		_gui.update(dt);
		_gui.frame_begin();
		
		if (_gui.main_menubar_begin())
		{
			_gui_menu_bar();
			_gui.main_menubar_end();
		}

		if (_player_window_state.open)
		{
			if (_gui.window_begin("Players", _player_window_state.open))
				_gui_players_window();
			_gui.window_end();
		}

		if (_level_window_state.open)
		{
			if (_gui.window_begin("Levels", _level_window_state.open))
				_gui_level_window();
			_gui.window_end();
		}

		if (_obj_w)
		{
			if (_gui.window_begin("Objects", _obj_w))
				_gui_object_window();
			_gui.window_end();
		}

		if (_obj_prop_w)
		{
			if (_gui.window_begin("Properties", _obj_prop_w))
				_obj_ui.value().object_properties(_gui);
			_gui.window_end();
		}

		//save load dialogs
		if (_save_window.open)
		{
			if (_gui.window_begin("Save", _save_window.open))
			{
				if (_gui.button("save"))
				{
					auto path = std::filesystem::path{ _save_window.path };
					if (!path.has_extension())
						path.replace_extension(_mission_ext->load());

					_save( std::move(path) );
					_save_window.open = false;
				}
				_gui.input("path", _save_window.path);
			}
			_gui.window_end();
		}

		if (_load_window.open)
		{
			if (_gui.window_begin("Load", _load_window.open))
			{
				if (_gui.button("load"))
					_load({ _load_window.path });
				_gui.input("path", _load_window.path);
			}
			_gui.window_end();
		}

		_gui.frame_end();
	}

	void mission_editor_t::_save(path sv_path)
	{
		auto m = mission{};
		m.description = _mission_desc;
		m.name = _mission_name;
	
		for (const auto& l : _levels)
		{
			if (l.path)
				m.external_levels.emplace_back(mission::external_level{ l.name, l.path.value() });
			else
                m.inline_levels.emplace_back(mission::level_element{ l.name, l.level_data });
		}

		for (const auto& p : _players)
		{
			mission::player player;
			player.id = p.id;
			player.object = p.object;
			m.players.emplace_back(std::move(player));
		}

		m.objects.next_id = _objects.next_id;
		m.objects.objects = _objects.objects;

		auto w = data::make_writer();
		serialise(m, *w);
		auto strm = files::write_file(sv_path);
		strm << w;
		_mission_src = std::move(sv_path);
	}

	void mission_editor_t::_load(path p)
	{
		auto strm = files::stream_file(p);
		if (!strm.is_open())
			return;//TODO: unable to find or read file gui message

		auto m = deserialise_mission(data::make_parser(strm));
		_mission_name = std::move(m.name);
		_mission_desc = std::move(m.description);

		_players = std::move(m.players);

		_objects.objects = m.objects.objects;
		_objects.next_id = m.objects.next_id;

		// iterate over objects and record the names into _objects.names
		for (const auto& obj : _objects.objects)
		{
			//if the object has a name then try and apply it
			const auto& name = obj.name_id;
			if (!empty(name))
			{
				//try and record the objects name
				auto [name_iter, name_already_used] = _objects.entity_names.try_emplace(name, obj.id);
				std::ignore = name_iter;

				//log error if duplicate names
				if (!name_already_used)
				{
					// TODO: gui message, unload level?
					using namespace std::string_literals;
					const auto msg = "error loading level, name: "s + name
						+ " has already been used by another entity"s;
					log_error(msg);
				}
			}
		}

		_levels.clear();
		for (auto& l : m.external_levels)
		{
			auto lev = level_info{};
			lev.name = std::move(l.name);
			lev.path = std::move(l.path);
			_levels.emplace_back(std::move(lev));
		}

		for (auto& l : m.inline_levels)
		{
			auto lev = level_info{};
			lev.name = std::move(l.name);
            lev.level_data = std::move(l.level_obj);
			_levels.emplace_back(std::move(lev));
		}

		_mission_src = p;
	}
}
