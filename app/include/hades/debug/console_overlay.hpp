#ifndef HADES_CONSOLE_OVERLAY_HPP
#define HADES_CONSOLE_OVERLAY_HPP

#include <functional>
#include <limits>

#include "hades/debug.hpp"
#include "hades/gui.hpp"
#include "hades/logging.hpp"
#include "hades/system.hpp"
#include "hades/types.hpp"

namespace hades::debug
{
	class console_overlay final : public basic_overlay
	{
	public:
		console_overlay();

		void update(gui&) override;
		void send_command();

		bool wants_close() const
		{
			return !_open;
		}

	private:
		void _text_callback(hades::gui_input_text_callback&);

		//input buffer
		string _input;

		//command history support
		console::command_history_list _history;
		std::size_t _history_index = std::numeric_limits<std::size_t>::max();

		//output display
		struct entry
		{
			string str;
			console::logger::log_verbosity verb;
		};
		console::property_int _log_mode;
		console::output_buffer _output;

		//tab completion
		std::vector<std::string_view> _completion_candidates;
		std::size_t _completion_index = {};

		//callback for closing the console
		bool _open = true;
	};
}

#endif //!HADES_CONSOLE_OVERLAY_HPP
