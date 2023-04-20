#include "hades/files.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <string>
#include <filesystem>

#include "hades/archive.hpp"
#include "hades/console_variables.hpp"
#include "hades/data.hpp"
#include "hades/logging.hpp"
#include "hades/properties.hpp"
#include "hades/standard_paths.hpp"
#include "hades/utility.hpp"

namespace fs = std::filesystem;

namespace hades::files 
{
	ofstream::ofstream() noexcept
		: std::ostream{ nullptr }
	{}

	ofstream::ofstream(const std::filesystem::path& p)
		: std::ostream{ nullptr }
	{
		open(p);
		return;
	}

	ofstream::ofstream(ofstream&& rhs) noexcept
		: std::ostream{ nullptr }
	{
		*this = std::move(rhs);
		return;
	}

	ofstream& ofstream::operator=(ofstream&& rhs) noexcept
	{
		_stream = std::move(rhs._stream);
		std::streambuf* str_ptr = {};
		std::visit([&str_ptr](auto&& stream) {
			str_ptr = &stream;
			return;
			}, _stream);

		rdbuf(str_ptr);
		rhs.rdbuf(nullptr);
		return *this;
	}

	void ofstream::open(const std::filesystem::path& p)
	{
		if (*console::get_bool(cvars::file_deflate, true))//cvars::default_value::file_deflate))
			_stream = zip::out_compressed_filebuf{ p };
		else
		{
			auto buf = std::filebuf{};
			buf.open(p, std::ios_base::out | std::ios_base::binary);
			_stream = std::move(buf);
		}

		std::streambuf* str_ptr = {};
		std::visit([&str_ptr](auto&& stream) {
			str_ptr = &stream;
			return;
			}, _stream);

		rdbuf(str_ptr);
		return;
	}

	void ofstream::close() noexcept
	{
		std::visit([](auto&& stream) {
			stream.close();
			return;
			}, _stream);
		return;
	}

	bool ofstream::is_open() const noexcept
	{
		return std::visit([](auto&& stream) {
			return stream.is_open();
			}, _stream);
	}
}

namespace hades
{
	irfstream::irfstream(irfstream&& rhs) noexcept
		: std::istream{ nullptr }
	{
		*this = std::move(rhs);
		return;
	}

	irfstream& irfstream::operator=(irfstream&& rhs) noexcept
	{
		std::swap(_stream, rhs._stream);
		std::swap(_mod_path, rhs._mod_path);
		std::swap(_rel_path, rhs._rel_path);
		_bind_buffer();
		setstate(rhs.rdstate());

		return *this;
	}

	void irfstream::open(const fs::path& m, const fs::path& f)
	{
		//check debug path
		#ifndef NDEBUG
		constexpr auto debug_path_str = HADES_ROOT_PROJECT_PATH "/game/";
		if(_try_open_from_dir({ debug_path_str }, m, f))
			return;
		#endif

		//check user path
		if(_try_open_from_dir(user_custom_file_directory(), m, f))
			return;

		//check app dir
		if (_try_open_from_dir(standard_file_directory(), m, f))
			return;

		throw files::file_not_found{ "Unable to find file: " + (m / f).generic_string() };
	}

	void irfstream::close() noexcept
	{
		std::visit([](auto&& stream) {
			stream.close();
			return;
		}, _stream);
		return;
	}

	bool irfstream::is_open() const noexcept
	{
		return std::visit([](auto& stream) {
			using T = std::decay_t<decltype(stream)>;
			if constexpr (std::is_same_v<T, zip::in_archive_filebuf>)
				return stream.is_open();
			else
				return stream.is_open();
			}, _stream);
	}

	bool irfstream::_try_open_from_dir(const fs::path& dir,
		const fs::path& mod, const fs::path& file)
	{
		auto m_path = dir / mod;
		if (fs::exists(m_path / file))
			return _try_open_file(m_path, file);

		if (!fs::exists(dir))
			return false;

		for (const auto& entry : fs::directory_iterator{ dir })
		{
			if (!entry.is_directory() && entry.path().stem() == mod)
			{
				if (_try_open_archive(entry.path(), file))
					return true;
			}
		}

		return false;
	}
	
	bool irfstream::_try_open_file(const fs::path& mod_path, const fs::path& file)
	{
		
		const auto set_stream = [&, this]<typename T>(T && stream)noexcept {
			_stream = std::forward<T>(stream);
			_mod_path = mod_path;
			_rel_path = file;
			_bind_buffer();
			clear();
		};

		auto c = zip::in_compressed_filebuf{};
		const auto path = mod_path / file;
		c.open(path);
		if (c.is_open())
		{
			set_stream(std::move(c));
			return true;
		}

		auto f = std::filebuf{};
		f.open(path, std::ios_base::in | std::ios_base::binary);
		if (f.is_open())
		{
			set_stream(std::move(f));
			return true;
		}

		return false;
	}

	bool irfstream::_try_open_archive(const fs::path& archive,
		const fs::path& file)
	{
		auto a = zip::in_archive_filebuf{};
		a.open_archive(archive);
		if (!a.is_open())
			return false;

		a.open(file);
		if (!a.is_open())
			return false;
		
		_stream = std::move(a);
		_mod_path = archive;
		_rel_path = file;
		_bind_buffer();
		clear();
		return true;
	}

	void irfstream::_bind_buffer() noexcept
	{
		std::streambuf* ptr = {};
		std::visit([&ptr](auto&& stream) {
			ptr = &stream;
			return;
			}, _stream);
		rdbuf(ptr);
		return;
	}
}

namespace hades::files
{
	irfstream stream_resource(const data::mod& m, const fs::path& path)
	{
		return irfstream{ { m.source }, path };
	}

	template<typename ReturnType, typename Stream>
	ReturnType from_stream(Stream& str)
	{
		if constexpr (std::is_same_v<ReturnType, string>)
		{
			auto str_stream = std::stringstream{};
			str_stream << str.rdbuf();
			return std::move(str_stream.str());
		}
		else if constexpr (std::is_same_v<ReturnType, buffer>)
		{
			str.seekg({}, std::ios_base::end);
			const auto size = str.tellg();
			str.seekg({}, std::ios_base::beg);

            // NOTE: fpos can be converted to streamoff but not size_t directly
            auto out = buffer(integer_cast<std::size_t>(static_cast<std::streamoff>(size)), {});
			str.get(reinterpret_cast<char*>(out.data()), size);
			return out;
		}
	}

	buffer raw_resource(const data::mod& m, const fs::path& path)
	{
		auto stream = stream_resource(m, path);
		return from_stream<buffer>(stream);
	}

	string read_resource(const data::mod& m, const fs::path& path)
	{
		auto str = stream_resource(m, path);
		return from_stream<string>(str);
	}

	static ifstream try_stream(const fs::path& first,
		const fs::path& second, const fs::path& path)
	{
		assert(path.is_relative());
		auto a = first / path;
		if (fs::exists(a))
			return ifstream{ std::move(a) };

		auto b = second / path;
		if (fs::exists(b))
			return ifstream{ std::move(b) };

		return {};
	}

	static buffer try_raw(const fs::path& first_path, const fs::path& second_path,
		const fs::path& file_name)
	{
		auto stream = try_stream(first_path, second_path, file_name);
		if(stream.is_open())
			return from_stream<buffer>(stream);
		return {};
	}

	static string try_read(const fs::path& first_path, const fs::path& second_path,
		const fs::path& file_name)
	{
		auto str = try_stream(first_path, second_path, file_name);
		if(str.is_open())
			return from_stream<string>(str);
		return {};
	}

	ifstream stream_file(const fs::path& p)
	{
		return try_stream(hades::user_custom_file_directory(), fs::current_path(), p);
	}

	string read_file(const fs::path& file_path)
	{
		return try_read(hades::user_custom_file_directory(), fs::current_path(), file_path);
	}

	buffer raw_file(const fs::path& file_path)
	{
		return try_raw(hades::user_custom_file_directory(), fs::current_path(), file_path);
	}

	string read_save(const fs::path& file_name)
	{
		return try_read(hades::user_save_directory(), standard_save_directory(), file_name);
	}

	string read_config(const fs::path& file_name)
	{
		return try_read(hades::user_config_directory(), standard_config_directory(), file_name);
	}

	bool make_directory(const fs::path& dir)
	{
		if (fs::exists(dir) && fs::is_directory(dir))
			return true;

		return fs::create_directories(dir);
	}

	static ofstream make_ofstream(const std::filesystem::path& p)
	{
		if (!p.has_filename())
		{
			const auto message = "unable to write file, path didn't include a filename; path was: " + p.generic_string();
			throw file_error{ message };
		}

		const auto parent = p.parent_path();
		if (!parent.empty() && !make_directory(parent))
		{
			const auto message = "No write permission for directory or unable to create directory; was: " + parent.generic_string();
			throw file_error{ message };
		}

		return ofstream{ p };
	}

	ofstream write_file(const std::filesystem::path& path)
	{
		auto p = path;
		if (!console::get_bool(cvars::file_portable, cvars::default_value::file_portable))
			p = user_custom_file_directory() / path;

		return make_ofstream(p);
	}

	void write_file(const fs::path& path, std::string_view file_contents)
	{
		auto stream = write_file(path);
        stream.write(reinterpret_cast<const char*>(std::data(file_contents)), integer_cast<std::streamsize>(std::size(file_contents)));
		return;
	}

	std::ofstream append_file_uncompressed(const std::filesystem::path& path)
	{
		// no way to append with our ofstream
		/*auto deflate = console::get_bool(cvars::file_deflate, cvars::default_value::file_deflate);
		const auto deflate_val = deflate->load();
		deflate->store(false);
		auto stream = make_ofstream(path);
		deflate->store(deflate_val);

		return stream;*/

		auto p = path;
		if(!console::get_bool(cvars::file_portable, cvars::default_value::file_portable))
			p = user_custom_file_directory() / path;

		if (!p.has_filename())
		{
			const auto message = "unable to write file, path didn't include a filename; path was: " + p.generic_string();
			throw file_error{ message };
		}

		const auto parent = p.parent_path();
		if (!parent.empty() && !make_directory(parent))
		{
			const auto message = "No write permission for directory or unable to create directory; was: " + parent.generic_string();
			throw file_error{ message };
		}

		return std::ofstream{ p, std::ios::app };
	}

	std::vector<types::string> ListFilesInDirectory(std::string_view dir_path)
	{
		std::vector<types::string> output;

		const fs::path dir(to_string(dir_path));
		if (!fs::is_directory(dir))
		{
			LOGERROR("\"" + to_string(dir_path) + "\" is not a directory");
			return output;
		}

		for (const auto& e : fs::directory_iterator(dir))
			output.push_back(e.path().filename().string());

		return output;
	}
}
