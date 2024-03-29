#include "hades/debug/console_overlay.hpp"

#include <algorithm>

#include "hades/console_variables.hpp"

using namespace std::string_view_literals;

namespace hades::debug
{
	constexpr auto console_input_flags =
		gui::input_text_flags::callback_completion |	// tab completion
		gui::input_text_flags::callback_edit |			// so we know when tab completion is abandoned
		gui::input_text_flags::callback_history |		// for recoving previous commands
		gui::input_text_flags::enter_returns_true;		// for submiting the input

	console_overlay::console_overlay()
		: _log_mode{ console::get_int(cvars::console_level, cvars::default_value::console_level) },
		_output{ console::output() }
	{}

	void console_overlay::update(gui& g)
	{
		constexpr auto mode_names = std::array{
			"Log(0)"sv,
			"Error(1)"sv,
			"Warning(2)"sv,
			"Debug(3)"sv
		};

		auto new_output = console::new_output();
		std::move(begin(new_output), end(new_output), back_inserter(_output));

		if (size(_output) > console::log_limit)
		{
			const auto beg = begin(_output);
			constexpr auto cull_count = console::log_limit - console::log_shrink_count;
			const auto end = next(beg, cull_count);
			_output.erase(beg, end);
		}

		constexpr auto child_window_name = "##console_child"sv;
		g.next_window_size({ 380.f,450.f }, gui::set_condition_enum::first_use);

		if (g.window_begin("Console##console_overlay", _open))
		{
			const auto appearing = g.is_window_appearing(); //store when we are first opening

			if (g.child_window_begin(child_window_name, {0.f, -g.get_frame_height_with_spacing()},
				true))
			{
				const auto auto_scroll = g.get_scroll_max_y() == g.get_scroll_y();
				const auto max_zero = g.get_scroll_max_y() == 0; //scrollbar doesnt exist yet
				const auto verb = console::logger::log_verbosity{ _log_mode->load() };
				auto has_new_output = false;
				for (const auto& s : _output)
				{
					const auto message_verb = s.verbosity();
					if (message_verb > verb)
						continue;
					using mode = console::logger::log_verbosity;
					if (message_verb == mode::error)
						g.push_colour(gui::colour_target::text, sf::Color::Red);
					else if (message_verb == mode::warning)
						g.push_colour(gui::colour_target::text, sf::Color::Yellow);

					has_new_output = true;

					g.push_text_wrap_pos(0.f);
					g.text(s.text());
					g.pop_text_wrap_pos();

					if(message_verb == mode::error ||
						message_verb == mode::warning)
						g.pop_colour();
				}

				if ((auto_scroll && has_new_output) || appearing || max_zero)
					g.set_scroll_here_y(1.f);
			}
			g.child_window_end();

			auto callback = [this](auto& param) {
				_text_callback(param);
				return;
			};

			if (appearing)
				g.set_keyboard_focus_here();

			if (g.input_text("##console_input", _input, console_input_flags, callback))
			{
				if(!empty(_input))
					send_command();
				g.activate_context(); // context may have been changed by send_command, eg. called vid_reinit
				g.set_keyboard_focus_here(-1);
			}

			g.layout_horizontal();

            const auto mode = _log_mode->load();
			const auto log_preview = [mode, &mode_names] {
				if (mode < 0)
					return mode_names[0];
				else if (mode > 3)
					return mode_names[3];
				else
                    return mode_names[integer_cast<std::size_t>(mode)];
			}();

			if (g.combo_begin("##console_level", log_preview))
			{
                for (auto i = std::size_t{}; i < 4; ++i)
				{
                    if (g.selectable(mode_names[i], i == integer_cast<std::size_t>(mode)))
                        _log_mode->store(integer_cast<int32>(i));
				}
				g.combo_end();
			}
		}

		g.window_end();
		return;
	}

	void console_overlay::send_command()
	{
		console::run_command(make_command(_input));
		_input.clear();
		_history = console::command_history();
		_history_index = size(_history);
		return;
	}

	void console_overlay::_text_callback(hades::gui_input_text_callback& input)
	{
		const auto callback_trigger = input.get_callback_trigger();
		if (callback_trigger == gui::input_text_flags::callback_completion)
		{
			if (empty(_completion_candidates))
			{
				_completion_index = {};
				//search for good candidates
				auto names = console::get_function_names();
				{
					auto vars = console::get_property_names();
					names.reserve(size(names) + size(vars));
					std::copy(begin(vars), end(vars), back_inserter(names));
				}

				const auto text = std::string_view{ input.input_contents() };
				names.erase(std::remove_if(begin(names), end(names), [text](auto name) {
					return name.find(text) == std::string_view::npos;
				}), end(names));

				std::partition(begin(names), end(names), [text](auto name) {
					return name.find(text) == std::string_view::size_type{};
				});

				_completion_candidates = std::move(names);
				if (!empty(_completion_candidates))
				{
					input.clear_chars();
					input.insert_chars(0, _completion_candidates[_completion_index]);
				}
			}
			else
			{
				// we already have a list to work from
				input.clear_chars();
				if (++_completion_index >= size(_completion_candidates))
					_completion_index = {};
				input.insert_chars(0, _completion_candidates[_completion_index]);
			}
		}//!callback tab
		else if (callback_trigger == gui::input_text_flags::callback_edit)
		{
			_completion_candidates.clear();
		}//!callback edit
		else if (callback_trigger == gui::input_text_flags::callback_history)
		{
			const auto key = input.get_input_key();
			if (key == gui_input_text_callback::input_key::up)
			{
				--_history_index;
				if (_history_index > size(_history) * 2)
					_history_index = size(_history) - 1;

				if (!empty(_history))
				{
					input.clear_chars();
					input.insert_chars(0, to_string(_history[_history_index]));
				}
			}
			else if (key == gui_input_text_callback::input_key::down)
			{
				++_history_index;
				const auto history_size = size(_history) - 1;
				if (_history_index > history_size)
					_history_index = 0;

				if (!empty(_history))
				{
					input.clear_chars();
					input.insert_chars(0, to_string(_history[_history_index]));
				}
			}
		}// !callback_history
		return;
	}
}
