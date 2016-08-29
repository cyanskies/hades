#ifndef HADES_STANDARDPATHS_HPP
#define HADES_STANDARDPATHS_HPP

#include <string>

// This is the directory containing additional content paths.
std::string getUserCustomFileDirectory();

// This directory is used for per user config files.
std::string getUserConfigDirectory();

// This is the dir that would be used for 
std::string getUserSaveDirectory();

#endif //HADES_STANDARDPATHS_HPP