#include "Hades\StandardPaths.hpp"

#include <assert.h>
#include <codecvt>
#include <sstream>

#include "Objbase.h"
#include "Shlobj.h"
#include "winerror.h"


//uses standard functions to get a named directory in windows.
// for list of valid KNOWN FOLDER ID's:
//https://msdn.microsoft.com/en-us/library/windows/desktop/dd378457(v=vs.85).aspx
std::string getWindowsDirectory(REFKNOWNFOLDERID target)
{
	PWSTR path = NULL;
	auto result = SHGetKnownFolderPath(target, 0, NULL, &path);

	if(result != S_OK)
	{
		assert("invalid arg or invalid folder ID in getUserCustomFileDirectory on windows");
	}

	std::wstringstream stream;
	stream << path;

	CoTaskMemFree(path);
	//stackoverflow voodoo to convert utf16 to utf8:
	// https://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
	return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(stream.str());	
}

//returns C:\\users\name\documents/
std::string getUserCustomFileDirectory()
{
	return getWindowsDirectory(FOLDERID_Documents) + "/";
}

