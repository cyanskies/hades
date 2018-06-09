#include "hades/standard_paths.hpp"

#ifdef _WIN32
#include "standard_paths_windows.cpp"
#endif //_WIN32

#ifdef __linux__
#include "standard_paths_linux.cpp"
#endif // __linux__


namespace hades
{
	types::string standard_file_directory()
	{
		return "./";
	}

	types::string standard_config_directory()
	{
		return "./config/";
	}

	types::string standard_save_directory()
	{
		return "./save/";
	}
}