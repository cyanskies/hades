#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "Hades/App.hpp"
#include "Hades/archive.hpp"
#include "Hades/Console.hpp"
#include "hades/files.hpp"
#include "hades/logging.hpp"
#include "Hades/Main.hpp"
#include "Hades/Types.hpp"

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
		hades::LoadCommand(commands, "compress", [](const hades::argument_list &command) {
			if (command.size() != 1)
			{
				LOGERROR("game command expects a single argument");
				return false;
			}
			hades::zip::compress_directory(hades::to_string(command.front()));
			return true;
		});

		hades::LoadCommand(commands, "uncompress", [](const hades::argument_list &command) {
			if (command.size() != 1)
			{
				LOGERROR("game command expects a single argument");
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

	std::at_quick_exit([] {
		//write log to file
		#ifdef NDEBUG
		auto logfile = hades::files::output_file_stream(std::filesystem::path{ hades::date() + ".txt" });
		#else
		auto logfile = std::ofstream(std::filesystem::path{ hades::date() + ".txt" }, std::ios_base::app);
		#endif
		assert(logfile.is_open());
		const auto log = hades::console::steal_output();
		for (const auto& str : log)
			logfile << static_cast<std::string>(str) << '\n';
		return;
		});

	app.postInit(commands);
	app.run();

	return EXIT_SUCCESS;
}
