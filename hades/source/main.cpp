#include <algorithm>
#include <string>
#include <vector>

#include "Hades/App.hpp"
#include "Hades/archive.hpp"
#include "Hades/Console.hpp"

int main(int argc, char** argv)
{
	//new commands start with a '-'.
	const char COMMAND_DELIMITER = '-';

	std::vector<std::string> cmdline;
	for(int i = 0; i < argc; ++i)
	{
		cmdline.push_back(argv[i]);
	}

	hades::CommandList commands;
	int index = -1;

	std::for_each(cmdline.begin(), cmdline.end(), [&commands, &index, COMMAND_DELIMITER] (std::string &s) {
		if(s[0] == COMMAND_DELIMITER)
		{
			s = s.substr(1, s.size());
			commands.push_back(s);
			index++;
		}
		else if(index != -1)
			commands[index] = commands[index] + " " + s;
	});

	auto command_length = commands.size();

	//===special commands===
	//these are invoked by using the engine as a normal command line program
	//the engine will not actually open if invoked with these commands
	try
	{
		hades::LoadCommand(commands, "compress", [](hades::CommandList::value_type param) {
			hades::zip::compress_directory(param);
		});

		hades::LoadCommand(commands, "uncompress", [](hades::CommandList::value_type param) {
			hades::zip::uncompress_archive(param);
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
		catch(hades::zip::archive_exception e)
		{
			//failed to read a resource from an archive //probably loading a mod.yaml
			LOGERROR(e.what());
		}

		app.cleanUp();
	}

	return returnCode;
}