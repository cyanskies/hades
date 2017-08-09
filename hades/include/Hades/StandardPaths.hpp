#ifndef HADES_STANDARDPATHS_HPP
#define HADES_STANDARDPATHS_HPP

#include "Hades/Types.hpp"

namespace hades
{
	// This is the directory containing additional content paths.
	// can return an empty string if no such location exists
	types::string GetUserCustomFileDirectory();

	// This directory is used for per user config files.
	// can return the ./config dir or some specilised user dir
	types::string GetUserConfigDirectory();

	// This is the dir that would be used for save files
	types::string GetUserSaveDirectory();
}

//for compatability
[[deprecated("Use hades::GetUserCustomFileDirectory() instead")]]
hades::types::string getUserCustomFileDirectory();


#endif //HADES_STANDARDPATHS_HPP