#ifndef HADES_STANDARDPATHS_HPP
#define HADES_STANDARDPATHS_HPP

#include <string>

// This is the directory containing additional content paths.
// can return an empty string if no such location exists
std::string getUserCustomFileDirectory();

// This directory is used for per user config files.
// can return the ./config dir or some specilised user dir
std::string getUserConfigDirectory();

// This is the dir that would be used for save files
std::string getUserSaveDirectory();

#endif //HADES_STANDARDPATHS_HPP