#include "hades/standard_paths.hpp"

#include <cassert>

#define NOMINMAX

#include "Objbase.h"
#include "Shlobj.h"
#include "winerror.h"

#include "hades/console_variables.hpp"
#include "hades/properties.hpp"
#include "hades/types.hpp"
#include "hades/utility.hpp"

static std::filesystem::path utf16_to_utf8(const PWSTR input)
{
	assert(input);

	//get buffer size by omitting output param
	const auto buffer_size = WideCharToMultiByte(CP_UTF8, 0, input, -1, NULL, 0, NULL, NULL);
	assert(buffer_size != 0);
	const auto buffer = std::make_unique_for_overwrite<char[]>(buffer_size);
	
	//write output into buffer
	const auto written_amount = WideCharToMultiByte(CP_UTF8, 0, input, -1, buffer.get(), buffer_size, NULL, NULL);
	assert(written_amount == buffer_size);
	std::ignore = written_amount;

	return std::filesystem::path{ buffer.get()};
}

//uses standard functions to get a named directory in windows.
// for list of valid KNOWN FOLDER ID's:
//https://msdn.microsoft.com/en-us/library/windows/desktop/dd378457(v=vs.85).aspx
static std::filesystem::path windows_directory(REFKNOWNFOLDERID target)
{
	PWSTR path = NULL;
	const auto result = SHGetKnownFolderPath(target, 0, NULL, &path);
	const auto finally = hades::make_finally([path]() noexcept {
		CoTaskMemFree(path);
	});

	if(result != S_OK)
		throw std::logic_error("invalid arg or invalid folder ID passed to SHGetKnownFolderPath");

	return utf16_to_utf8(path);
}

namespace hades
{
	using namespace std::string_literals;

	static types::string game()
	{
		const auto name = hades::console::get_string(cvars::game_vanity_name, cvars::default_value::game_vanity_name);
		return name->load() + "/";
	}

	//returns C:\\users\\<name>\\documents/gamename/
	std::filesystem::path user_custom_file_directory()
	{
		static const auto portable = hades::console::get_bool("file_portable", false);

		if (*portable)
			return standard_file_directory();
		else
			return windows_directory(FOLDERID_Documents) / game();
	}

	std::filesystem::path user_config_directory()
	{
		static const auto portable = hades::console::get_bool("file_portable", false);

		//if portable is defined, then load read only config from application root directory
		if (*portable)
			return standard_config_directory();
		else
			return windows_directory(FOLDERID_Documents) / game() / standard_config_directory();
	}

	std::filesystem::path user_save_directory()
	{
		static const auto portable = hades::console::get_bool("file_portable", false);

		using namespace std::string_literals;
		if (*portable)
			return standard_save_directory();
		else
			return windows_directory(FOLDERID_SavedGames) / game() / standard_save_directory();
	}
}
