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
	void irfstream::open(const fs::path& m, const fs::path& f)
	{
		//check debug path
		#ifndef NDEBUG
		constexpr auto debug_path_str = "../../game/";
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

	struct is_open_functor
	{
		bool operator()(const files::ifstream& s) noexcept
		{
			return s.is_open();
		}

		bool operator()(const zip::iafstream& a) noexcept
		{
			return a.is_file_open();
		}
	};

	bool irfstream::is_open() const noexcept
	{
		return std::visit(is_open_functor{}, _stream);
	}

	irfstream& irfstream::read(char_t* d, std::size_t c)
	{
		std::visit([d, c](auto&& stream) {
			using T = std::decay_t<decltype(stream)>;
			stream.read(reinterpret_cast<T::char_type*>(d), c);
			return;
		}, _stream);

		return *this;
	}

	std::streamsize irfstream::gcount() const
	{
		return std::visit([](auto&& stream) {
			return stream.gcount();
		}, _stream);
	}

	irfstream::pos_type irfstream::tellg()
	{
		return std::visit([](auto&& stream) {
			return stream.tellg();
		}, _stream);
	}

	bool irfstream::eof() const
	{
		return std::visit([](auto&& stream)	{
				return stream.eof();
		}, _stream);
	}

	void irfstream::seekg(pos_type p)
	{
		std::visit([p](auto&& stream) {
			stream.seekg(p);
			return;
		}, _stream);

		return;
	}

	void irfstream::seekg(off_type o, std::ios_base::seekdir s)
	{
		std::visit([o, s](auto&& stream) {
			stream.seekg(o, s);
			return;
		}, _stream);

		return;
	}

	std::size_t irfstream::size()
	{
		const auto pos = tellg();
		seekg({}, std::ios_base::end);
		const auto size = tellg();
		seekg(pos);
		return integer_cast<std::size_t>(static_cast<std::streamoff>(size));
	}

	bool irfstream::_try_open_from_dir(const fs::path& dir,
		const fs::path& mod, const fs::path& file)
	{
		auto m_path = dir / mod;
		if (fs::exists(m_path / file))
			return _try_open_file(m_path, file);

		if (fs::exists(m_path) && !fs::is_directory(m_path)
			&& zip::file_exists(m_path, file))
			return _try_open_archive(std::move(m_path), file);

		return false;
	}
	
	bool irfstream::_try_open_file(const fs::path& mod_path, const fs::path& file)
	{
		auto f = files::ifstream{ mod_path / file };
		if (f.is_open())
		{
			_stream = std::move(f);// = std::move(f);
			_mod_path = mod_path;
			_rel_path = file;
			return true;
		}

		return false;
	}

	bool irfstream::_try_open_archive(fs::path archive,
		const fs::path& file)
	{
		auto a = zip::iafstream{ archive, file };
		if (a.is_file_open())
		{
			_stream = std::move(a);
			_mod_path = std::move(archive);
			_rel_path = file;
			return true;
		}
		return false;
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
		str.seekg({}, std::ios_base::end);
		const auto size = integer_cast<std::size_t>(static_cast<std::streamoff>(str.tellg()));
		str.seekg({}, std::ios_base::beg);

		auto out = ReturnType{};
		out.reserve(size);

		auto buf = buffer{ default_buffer_size };
		while (std::size(out) != size)
		{
			if (str.eof())
				break;

			str.read(reinterpret_cast<Stream::char_type*>(std::data(buf)), default_buffer_size);
			
			const auto beg = std::begin(buf);
			using diff_t = typename std::iterator_traits<std::decay_t<decltype(beg)>>::difference_type;
			auto end = std::next(beg, integer_cast<diff_t>(str.gcount()));

			if constexpr(std::is_same_v<buffer::value_type, typename ReturnType::value_type>)
				out.insert(std::end(out), beg, end);
			else
			{
				std::transform(beg, end, std::back_inserter(out), [](const std::byte b) {
					return static_cast<typename ReturnType::value_type>(b);
					});
			}
		}

		return out;
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
		stream.write(reinterpret_cast<const char*>(std::data(file_contents)), std::size(file_contents));
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
