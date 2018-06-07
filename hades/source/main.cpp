#include <algorithm>
#include <string>
#include <vector>

#include "Hades/App.hpp"
#include "Hades/archive.hpp"
#include "Hades/Console.hpp"
#include "Hades/Main.hpp"
#include "Hades/Types.hpp"

int hades_main(int argc, char* argv[])
{
	//new commands start with a '-'.
	constexpr char COMMAND_DELIMITER = '-';

	std::vector<std::string_view> cmdline;
	cmdline.reserve(argc);

	//convert cmdline to string_view
	std::copy(argv, argv + argc, std::back_inserter(cmdline));

	hades::command_list commands;
	int index = -1;

	for (auto &s : cmdline)
	{
		if (s[0] == COMMAND_DELIMITER)
		{
			s = s.substr(1, s.size());
			hades::command com;
			com.request = s;
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
	catch (hades::zip::archive_exception &e)
	{
		LOGERROR(e.what());
		return EXIT_FAILURE;
	}

	if (commands.size() != command_length)
		return EXIT_SUCCESS;

	int returnCode = EXIT_FAILURE;
	{
		hades::App app;

		app.init();
		try
		{
			app.postInit(commands);

			returnCode = app.run();
		}
		catch(hades::zip::archive_exception &e)
		{
			//failed to read a resource from an archive //probably loading a mod.yaml
			LOGERROR(e.what());
		}

		app.cleanUp();
	}

	return returnCode;
}
