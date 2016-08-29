
#include <algorithm>
#include <string>
#include <vector>

#include "Hades/App.hpp"

int main(int argc, char** argv)
{
	//new commands start with a '-'.
	const char COMMAND_DELIMITER = '-';

	std::vector<std::string> cmdline;
	for(int i = 0; i < argc; ++i)
	{
		cmdline.push_back(argv[i]);
	}

	std::vector<std::string> commands;
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

	int returnCode = EXIT_FAILURE;
	{
		hades::App app;

		app.init();
		app.postInit(commands);

		returnCode = app.run();

		app.cleanUp();
	}

	return returnCode;
}