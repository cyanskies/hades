#include "hades/archive_streams.hpp"

#include <fstream>
#include <set>

#include "hades/string.hpp"

namespace fs = std::filesystem;

using namespace std::string_literals;
using namespace std::string_view_literals;

//this is the default archive extension
//used when created archives, but not when searching for them
constexpr auto archive_ext = ".zip"sv;

namespace hades::zip
{
	// list of open archive handles
	static std::multiset<string> open_for_read;
	static std::set<string> open_for_write;

	unarchive open_archive(const fs::path& path)
	{
		auto path_str = path.generic_string();
		if (open_for_write.contains(path_str))
			throw files::file_error{ "Cannot open archive: " + path_str + ", it is already open for writing"s };

		if (!fs::exists(path))
			throw files::file_not_found{ "archive not found: "s + path.generic_string() };
		//open archive
		auto zip = unzOpen64(path_str.c_str());

		if (!zip)
			return {};

		open_for_read.emplace(path_str);

		return { path, {}, zip };
	}

	void open_file(unarchive& a, const std::filesystem::path& f)
	{
		assert(a.handle);

		//sets current file to the target if it returns true
		if (!file_exists(a, f))
			throw file_not_found{ "file not found in archive: "s + f.generic_string() };

		if (unzOpenCurrentFile(a.handle) != UNZ_OK)
			throw archive_error{ "error opening file in archive"s };

		a.file = f;
		return;
	}

	void open_first_file(unarchive& a)
	{
		assert(a.file.empty());
		if (UNZ_OK != unzGoToFirstFile(a.handle))
			throw archive_error{ "error opening file in archive"s };

		if (unzOpenCurrentFile(a.handle) != UNZ_OK)
			throw archive_error{ "error opening file in archive"s };

		auto file_info = unz_file_info64{};
		unzGetCurrentFileInfo64(a.handle, &file_info, nullptr, 0, nullptr, 0, nullptr, 0);

		auto character_buffer = std::string(file_info.size_filename, '\0');
		unzGetCurrentFileInfo64(a.handle, nullptr, character_buffer.data(),
			integer_cast<uLong>(size(character_buffer)), nullptr, 0, nullptr, 0);

		a.file = character_buffer;
		return;
	}

	bool open_next_file(unarchive& a)
	{
		if (const auto next = unzGoToNextFile(a.handle); next == UNZ_END_OF_LIST_OF_FILE)
			return false;
		else if(next != UNZ_OK)
			throw archive_error{ "error opening file in archive"s };

		if (unzOpenCurrentFile(a.handle) != UNZ_OK)
			throw archive_error{ "error opening file in archive"s };

		auto file_info = unz_file_info64{};
		unzGetCurrentFileInfo64(a.handle, &file_info, nullptr, 0, nullptr, 0, nullptr, 0);

		auto character_buffer = std::string(file_info.size_filename, '\0');
		unzGetCurrentFileInfo64(a.handle, nullptr, character_buffer.data(),
			integer_cast<uLong>(size(character_buffer)), nullptr, 0, nullptr, 0);

		a.file = character_buffer;
		return true;
	}

	void close_file(unarchive& a)
	{
		assert(a.handle);
		assert(!a.file.empty());
		a.file.clear();
		if (unzCloseCurrentFile(a.handle) == UNZ_CRCERROR)
			throw archive_error{ "CRC error reading file: " + a.file.generic_string() };
		return;
	}

	void close_archive(unarchive& f) noexcept
	{
		assert(f.handle);
		const auto r = unzClose(f.handle);
		if (r != UNZ_OK)
			log_error("Error while closing archive"sv);

		const auto iter = open_for_read.find(f.path.generic_string());
		assert(iter != end(open_for_read));
		open_for_read.erase(iter);
		f = {};

		return;
	}

	constexpr int case_sensitivity_auto = 0,//no sensitivity on windows
		case_sensitivity_sensitive = 1,		//case sensitivity everywhere
		case_sensitivity_none = 2; 			//no sensitivity on any platform

	bool file_exists(const unarchive& a, const std::filesystem::path& path) noexcept
	{
		assert(a.handle);
		const auto path_str = path.generic_string();
		const auto r = unzLocateFile(a.handle, path_str.c_str(), case_sensitivity_sensitive);
		return r == UNZ_OK;
	}

	toarchive create_archive(const std::filesystem::path& path)
	{
		auto path_str = path.generic_string(); 
		if (open_for_read.contains(path_str) || open_for_write.contains(path_str))
			throw files::file_error{ "Cannot open archive: " + path_str + ", for writing; it is already being used"s };

		auto a = toarchive{ path_str };
		if (!fs::exists(path))
		{
			a.handle = zipOpen(path_str.c_str(), APPEND_STATUS_CREATE);
			if(a.handle)
				log_debug("Created new archive: "s + path_str);
		}
		else
		{
			const auto file = std::ofstream{ path }; // hold the file
			if (file)
			{
				if (fs::file_size(path) != 0)
				{
					try
					{
						fs::resize_file(path, 0);
					}
					catch (const fs::filesystem_error& e)
					{
						throw files::file_error{ e.what() };
					}
				}
			}
			else
				throw files::file_error{ "Cannot open file: "s + path.generic_string() };

			a.handle = zipOpen(path_str.c_str(), APPEND_STATUS_ADDINZIP);
			if(a.handle)
				log_debug("Overwriting archive: "s + path_str);
		}

		if (!a.handle)
			throw archive_error{ "unable to create archive: "s + path.generic_string() };

		open_for_write.emplace(std::move(path_str));
		return a;
	}

	void close_archive(toarchive f) noexcept
	{
		const auto r = zipClose(f.handle, {});
		if (r != UNZ_OK)
			log_error("tried to close archive before closing file"sv);

		open_for_write.erase(f.path);
		return;
	}
}
