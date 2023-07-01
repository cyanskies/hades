#include "hades/mod_editor.hpp"

namespace hades::detail
{
	bool mod_exists(std::string_view name) noexcept
	{
		try
		{
			auto root = std::filesystem::path{ "."sv };

			if (!console::get_bool(cvars::file_portable, cvars::default_value::file_portable))
				root = user_custom_file_directory();

			auto filename = std::filesystem::path{ name };
			auto deflate = console::get_bool(cvars::file_deflate, cvars::default_value::file_deflate);
			if (filename.has_extension())
				return std::filesystem::exists(root / filename);

			filename.replace_extension(zip::resource_archive_ext());
			if (std::filesystem::exists(root / filename))
				return true;

			filename.replace_extension();
			return std::filesystem::is_directory(root / filename);
		}
		catch (const std::exception& exc)
		{
			log_warning(exc.what());
			return true;
		}
		catch (...)
		{
			log_error("Unexpected error"sv);
			return true;
		}
	}
}
