#ifndef HADES_STANDARDPATHS_HPP
#define HADES_STANDARDPATHS_HPP

#include "hades/types.hpp"

namespace hades
{
	// This is the directory containing additional content paths.
	// can return an empty string if no such location exists
	types::string user_custom_file_directory();

	// This directory is used for per user config files.
	// can return the ./config dir or some specilised user dir
	types::string user_config_directory();

	// This is the dir that would be used for save files
	types::string user_save_directory();
}

#endif //HADES_STANDARDPATHS_HPP