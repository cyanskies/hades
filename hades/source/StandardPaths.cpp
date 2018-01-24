#include "Hades/StandardPaths.hpp"

#ifdef _WIN32
#include "StandardPathsWindows.cpp"
#endif //_WIN32

#ifdef __linux__
#include "StandardPathsLinux.cpp"
#endif // __linux__
