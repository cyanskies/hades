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

	return hades::types::string{ buffer.data() };
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
	using namespace std::string_literals;

	//returns C:\\users\\<name>\\documents/gamename/
	types::string GetUserCustomFileDirectory()
	{
		static const auto portable = hades::console::GetBool("file_portable", false);

		if (*portable)
			return "./"s;
		else
			return getWindowsDirectory(FOLDERID_Documents) + "/"s + to_string(defaultGame()) + "/"s;
	}

	types::string GetUserConfigDirectory()
	{
		static const auto portable = hades::console::GetBool("file_portable", false);

		//if portable is defined, then load read only config from application root directory
		if (*portable)
			return "./"s;
		else
		{
			const auto postfix = "/"s + to_string(defaultGame()) + "/config/"s;
			return getWindowsDirectory(FOLDERID_Documents) + postfix;
		}
	}

	types::string GetUserSaveDirectory()
	{
		static const auto portable = hades::console::GetBool("file_portable", false);

		using namespace std::string_literals;
		if (*portable)
			return "./save/"s;
		else
			return getWindowsDirectory(FOLDERID_SavedGames) + "/"s + to_string(defaultGame()) + "/"s;
	}
}
