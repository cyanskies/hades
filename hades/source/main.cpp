#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "hades/App.hpp"
#include "hades/archive.hpp"
#include "hades/Console.hpp"
#include "hades/files.hpp"
#include "hades/logging.hpp"
#include "hades/Main.hpp"
#include "hades/types.hpp"

using namespace std::string_view_literals;

void try_write_log()
{
	if (hades::console::is_logging())
		hades::console::dump_log();
	return;
}

int hades::hades_main(int argc, char* argv[], std::string_view game,
	register_resource_types_fn resource_fn,
	app_main_fn app_fn)
{
	std::ios_base::sync_with_stdio(false);
	//new commands start with a '-'.
	constexpr char COMMAND_DELIMITER = '-';

	//convert cmdline to string_view
	const auto cmdline = std::vector<std::string_view>{ argv, argv + argc };

	hades::command_list commands;
	int index = -1;

	for (const auto s : cmdline)
	{
		if (s[0] == COMMAND_DELIMITER)
		{
			const auto s2 = s.substr(1, s.size());
			hades::command com;
			com.request = s2;
			commands.push_back(com);
			index++;
		}
		else if (index != -1)
			commands[index].arguments.push_back(s);
	}

	const auto command_length = commands.size();

	//===special commands===
	//these are invoked by using the engine as a normal command line program
	//the engine will not actually open if invoked with these commands
	try
	{
		hades::handle_command(commands, "compress"sv, [](const hades::argument_list& command) {
			if (command.size() != 1)
			{
				LOGERROR("game command expects a single argument"sv);
				return false;
			}
			hades::zip::compress_directory(hades::to_string(command.front()));
			return true;
			});

		hades::handle_command(commands, "uncompress"sv, [](const hades::argument_list& command) {
			if (command.size() != 1)
			{
				LOGERROR("game command expects a single argument"sv);
				return false;
			}
			hades::zip::uncompress_archive(hades::to_string(command.front()));
			return true;
			});
	}
	catch (hades::files::file_error& e)
	{
		LOGERROR(e.what());
		return EXIT_FAILURE;
	}

	// we handled and ran a proccess in main, so don't start the normal app
	if (commands.size() != command_length)
		return EXIT_SUCCESS;

	auto app = hades::App{};

	app.init(game, resource_fn);

	std::at_quick_exit(try_write_log);

	try
	{
		app.postInit(commands, app_fn);
		app.run();
	}
	catch (const std::exception& e)
	{
		hades::log_error("Unhandled exception"sv);
		hades::log_error(e.what());
		hades::console::dump_log();
		throw;
	}
	catch (...)
	{
		hades::log_error("Unexpected exception"sv);
		hades::console::dump_log();
		throw;
	}

	return EXIT_SUCCESS;
}
