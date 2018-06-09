#include "hades/standard_paths.hpp"

#ifdef _WIN32
#include "standard_paths_windows.cpp"
#endif //_WIN32

#ifdef __linux__
#include "standard_paths_linux.cpp"
#endif // __linux__
