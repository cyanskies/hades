#include "Hades/StandardPaths.hpp"

hades::types::string getUserCustomFileDirectory()
{
	return hades::GetUserCustomFileDirectory();
}

#ifdef _WIN32
#include "StandardPathsWindows.cpp"
#endif

#ifdef linux
#include "StandardPathsLinux.cpp"
#endif