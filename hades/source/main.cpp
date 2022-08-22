#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "hades/App.hpp"
#include "hades/archive.hpp"
#include "hades/Console.hpp"
#include "hades/files.hpp"
#include "hades/logging.hpp"
#include "Hades/Main.hpp"
#include "Hades/Types.hpp"

using namespace std::string_view_literals;

template<bool Steal = true>
std::conditional_t<Steal, void, bool> write_log()
{
	//write log to file
	#ifdef NDEBUG
	auto logfile = hades::files::output_file_stream(std::filesystem::path{ hades::date() + ".txt" });
	#else
	auto logfile = std::ofstream(std::filesystem::path{ hades::date() + ".txt" }, std::ios_base::app);
	#endif
	assert(logfile.is_open());
	
	auto log = hades::console::output_buffer{};
	if constexpr (Steal)
		log = hades::console::steal_output();
	else
		log = hades::console::copy_output();

	for (const auto& str : log)
		logfile << static_cast<std::string>(str) << '\n';

	if constexpr (Steal)
		return;
	else
		return true;
}

int hades_main(int argc, char* argv[])
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
		hades::LoadCommand(commands, "compress"sv, [](const hades::argument_list &command) {
			if (command.size() != 1)
			{
				LOGERROR("game command expects a single argument"sv);
				return false;
			}
			hades::zip::compress_directory(hades::to_string(command.front()));
			return true;
		});

		hades::LoadCommand(commands, "uncompress"sv, [](const hades::argument_list &command) {
			if (command.size() != 1)
			{
				LOGERROR("game command expects a single argument"sv);
				return false;
			}
			hades::zip::uncompress_archive(hades::to_string(command.front()));
			return true;
		});
	}
	catch (hades::files::file_error &e)
	{
		LOGERROR(e.what());
		return EXIT_FAILURE;
	}

	// we handled and ran a proccess in main, so don't start the normal app
	if (commands.size() != command_length)
		return EXIT_SUCCESS;

	auto app = hades::App{};

	app.init();
	hades::console::add_function("write_log"sv, write_log<false>, true);

	#ifndef NDEBUG
	std::at_quick_exit(write_log);
	#endif

	try
	{
		app.postInit(commands);
		app.run();
	}
	catch(const std::exception &e)
	{
		hades::log_error("Unhandled exception"sv);
		hades::log_error(e.what());
		write_log();
		throw;
	}
	catch (...)
	{
		hades::log_error("Unexpected exception"sv);
		write_log();
		throw;
	}

	return EXIT_SUCCESS;
}
