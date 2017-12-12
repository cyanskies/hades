#include "Hades/StandardPaths.hpp"

#include <cassert>

#include "Objbase.h"
#include "Shlobj.h"
#include "winerror.h"

#include "Hades/Properties.hpp"
#include "Hades/Main.hpp"
#include "Hades/Types.hpp"

hades::types::string Utf16ToUtf8(std::wstring input)
{
	assert(!input.empty());

	//get buffer size by omitting output param
	auto buffer_size = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, NULL, 0, NULL, NULL);
	std::vector<char> buffer(buffer_size);
	//write output into buffer
	auto written_amount = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, buffer.data(), buffer.size(), NULL, NULL);
	assert(written_amount == buffer_size);

	//copy output into a string
	hades::types::string out;
	std::copy(std::begin(buffer), std::end(buffer), std::back_inserter(out));

	return out;
}

//uses standard functions to get a named directory in windows.
// for list of valid KNOWN FOLDER ID's:
//https://msdn.microsoft.com/en-us/library/windows/desktop/dd378457(v=vs.85).aspx
hades::types::string getWindowsDirectory(REFKNOWNFOLDERID target)
{
	PWSTR path = NULL;
	auto result = SHGetKnownFolderPath(target, 0, NULL, &path);

	if(result != S_OK)
	{
		assert(false);
		throw std::logic_error("invalid arg or invalid folder ID in getUserCustomFileDirectory on windows");
	}

	std::wstring str(path);

	CoTaskMemFree(path);
	return Utf16ToUtf8(str);
}

namespace hades
{
	//returns C:\\users\\<name>\\documents/gamename/
	types::string GetUserCustomFileDirectory()
	{
		static const auto portable = hades::console::GetBool("file_portable", false);

		if (*portable)
			return "";
		else
			return getWindowsDirectory(FOLDERID_Documents) + "/" + defaultGame() + "/";
	}
}
