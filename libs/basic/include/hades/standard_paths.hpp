#ifndef HADES_STANDARDPATHS_HPP
#define HADES_STANDARDPATHS_HPP

#include <filesystem>

#include "hades/types.hpp"

namespace hades
{
	//use the user_* versions, the standard_ versions are for
	//loading configs or save's that are distributed with the app

	// This is the directory containing additional content paths.
	// can return an empty string if no such location exists
	std::filesystem::path user_custom_file_directory();
	std::filesystem::path standard_file_directory();
	// This directory is used for per user config files.
	// can return the ./config dir or some specilised user dir
	std::filesystem::path user_config_directory();
	std::filesystem::path standard_config_directory();

	// This is the dir that would be used for save files
	std::filesystem::path user_save_directory();
	std::filesystem::path standard_save_directory();
}

#endif //HADES_STANDARDPATHS_HPP