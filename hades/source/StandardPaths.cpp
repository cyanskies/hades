#include "Hades/StandardPaths.hpp"

#ifdef _WIN32
#include "StandardPathsWindows.cpp"
#endif

#ifdef linux
#include "StandardPathsLinux.cpp"
#endif