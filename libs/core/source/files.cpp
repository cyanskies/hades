#include "Hades/files.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <string>
#include <filesystem>

#include "hades/archive.hpp"
#include "hades/logging.hpp"
#include "hades/standard_paths.hpp"
#include "hades/utility.hpp"

namespace fs = std::filesystem;

namespace hades::files
{
	ifstream::ifstream(const std::filesystem::path& p)
	{
		open(p);
		return;
	}

	void ifstream::open(const std::filesystem::path& p)
	{
		if (!std::filesystem::exists(p))
			throw file_not_found{ "file not found: " + p.string() };
		auto f = stream_t{ p, std::ios::in | std::ios::binary };
		auto h = zip::zip_header{};
		using char_type = ifstream::stream_t::char_type;
		f.read(reinterpret_cast<char_type*>(std::data(h)), std::size(h));
		f.seekg({}, std::ios_base::beg);
		if (zip::probably_compressed(h))
			_stream.emplace<zip::izfstream>(std::move(f));
		else
			_stream = std::move(f);
		return;
	}

	void ifstream::close() noexcept
	{
		std::visit([](auto&& stream) {
			stream.close();
			return;
			}, _stream);

		return;
	}

	bool ifstream::is_open() const noexcept
	{
		return std::visit([](const auto& stream)->bool {
			return stream.is_open();
			}, _stream);
	}

	struct read_visitor
	{};

	ifstream& ifstream::read(char_t* ptr, std::size_t size)
	{
		std::visit([ptr, size](auto&& stream) {
			using T = std::decay_t<decltype(stream)>;
			if constexpr(std::is_same_v<T, std::ifstream>)
				stream.read(reinterpret_cast<std::ifstream::char_type*>(ptr), size);
			else
				stream.read(ptr, size);
			return;
			}, _stream);
		return *this;
	}

	std::streamsize ifstream::gcount() const
	{
		return std::visit([](auto&& stream)->std::streamsize const {
			return stream.gcount();
			}, _stream);
	}

	ifstream::pos_type ifstream::tellg()
	{
		return std::visit([](auto&& stream)->pos_type {
			return stream.tellg();
			}, _stream);
	}

	bool ifstream::eof() const
	{
		return std::visit([](const auto& stream)->bool {
			return stream.eof();
			}, _stream);
	}

	void ifstream::seekg(pos_type p)
	{
		std::visit([p](auto&& stream) {
			stream.seekg(p);
			return;
			}, _stream);
		return;
	}

	void ifstream::seekg(off_type o, std::ios_base::seekdir dir)
	{
		std::visit([o, dir](auto&& stream) {
			stream.seekg(o, dir);
			return;
			}, _stream);
		return;
	}
}

namespace hades
{
	irfstream::irfstream(const std::filesystem::path& mod, const std::filesystem::path& file)
	{
		open(mod, file);
		return;
	}

	void irfstream::open(const std::filesystem::path& m, const std::filesystem::path& f)
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

		throw files::file_not_found{ "unable to find file: " + (m / f).generic_string() };
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
			stream.read(d, c);
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

	bool irfstream::_try_open_from_dir(const std::filesystem::path& dir,
		const std::filesystem::path& mod, const std::filesystem::path& file)
	{
		const auto f_path = dir / mod / file;
		if (fs::exists(f_path))
			return _try_open_file(f_path);

		const auto archive_path = dir / mod;
		if (fs::exists(archive_path) && !fs::is_directory(archive_path)
			&& zip::file_exists(archive_path, file))
			return _try_open_archive(archive_path, file);

		return false;
	}
	
	bool irfstream::_try_open_file(const std::filesystem::path& file)
	{
		auto f = files::ifstream{ file };
		const auto open = f.is_open();
		if (open)
			_stream = std::move(f);

		return open;
	}
	bool irfstream::_try_open_archive(const std::filesystem::path& archive,
		const std::filesystem::path& file)
	{
		auto a = zip::iafstream{ archive, file };
		const auto open = a.is_file_open();
		if(open)
			_stream = std::move(a);
		return open;
	}
}

namespace hades::files
{
	irfstream stream_resource(const resources::mod* m, const std::filesystem::path& path)
	{
		assert(m);
		return irfstream{ { m->source }, path };
	}

	template<typename ReturnType, typename Stream>
	ReturnType from_stream(Stream& str)
	{
		str.seekg({}, std::ios_base::end);
		const auto size = integer_cast<std::size_t>(static_cast<std::streamoff>(str.tellg()));
		str.seekg({}, std::ios::beg);

		auto out = ReturnType{};
		out.reserve(size);

		auto buf = buffer{ default_buffer_size };
		while (std::size(out) != size)
		{
			if (str.eof())
				break;

			str.read(std::data(buf), default_buffer_size);
			
			const auto beg = std::begin(buf);
			using diff_t = typename std::iterator_traits<std::decay_t<decltype(beg)>>::difference_type;
			auto end = std::next(beg, integer_cast<diff_t>(str.gcount()));

			if constexpr(std::is_same_v<buffer::value_type, typename ReturnType::value_type>)
				std::copy(beg, end, std::back_inserter(out));
			else
			{
				std::transform(beg, end, std::back_inserter(out), [](const std::byte b) {
					return static_cast<typename ReturnType::value_type>(b);
					});
			}
		}

		return out;
	}

	buffer raw_resource(const resources::mod* m, const std::filesystem::path& path)
	{
		auto stream = stream_resource(m, path);
		return from_stream<buffer>(stream);
	}

	string read_resource(const resources::mod* m, const std::filesystem::path& path)
	{
		auto str = stream_resource(m, path);
		return from_stream<string>(str);
	}

	static ifstream try_stream(const std::filesystem::path& first,
		const std::filesystem::path& second, const std::filesystem::path& path)
	{
		assert(path.is_relative());
		auto a = first / path;
		if (std::filesystem::exists(a))
			return ifstream{ std::move(a) };

		auto b = second / path;
		if (std::filesystem::exists(b))
			return ifstream{ std::move(b) };

		return {};
	}

	static buffer try_raw(const std::filesystem::path& first_path, const std::filesystem::path& second_path,
		const std::filesystem::path& file_name)
	{
		auto stream = try_stream(first_path, second_path, file_name);
		return from_stream<buffer>(stream);
	}

	static string try_read(const std::filesystem::path& first_path, const std::filesystem::path& second_path,
		const std::filesystem::path& file_name)
	{
		auto str = try_stream(first_path, second_path, file_name);
		return from_stream<string>(str);
	}

	ifstream stream_file(const std::filesystem::path& p)
	{
		return try_stream(hades::user_custom_file_directory(), std::filesystem::current_path(), p);
	}

	string read_file(const std::filesystem::path& file_path)
	{
		return try_read(hades::user_custom_file_directory(), fs::current_path(), file_path);
	}

	buffer raw_file(const std::filesystem::path& file_path)
	{
		return try_raw(hades::user_custom_file_directory(), fs::current_path(), file_path);
	}

	string read_save(const std::filesystem::path& file_name)
	{
		return try_read(hades::user_save_directory(), standard_save_directory(), file_name);
	}

	string read_config(const std::filesystem::path& file_name)
	{
		return try_read(hades::user_config_directory(), standard_config_directory(), file_name);
	}

	bool make_directory(const fs::path& dir)
	{
		if (fs::exists(dir) && fs::is_directory(dir))
			return true;

		return fs::create_directories(dir);
	}

	void write_file(const fs::path& path, std::string_view file_contents)
	{
		const auto p = user_custom_file_directory() / path;

		if (!p.has_filename())
		{
			const auto message = "unable to write file, path didn't include a filename; path was: " + p.generic_string();
			throw file_error{ message };
		}

		const auto parent = p.parent_path();
		if (!make_directory(parent))
		{
			const auto message = "No write permission for directory or unable to create directory; was: " + parent.generic_string();
			throw file_error{ message };
		}

		//TODO: stream deflation
		// we dont want to hold the whole stream in memory
		// even though our input is
		auto as_bytes = buffer{};

		std::transform(std::begin(file_contents), std::end(file_contents), std::back_inserter(as_bytes), [](const auto c) {
			return static_cast<std::byte>(c);
			});

		const auto output = zip::deflate(std::move(as_bytes));

		std::ofstream file{ p, std::ios::binary | std::ios::trunc };
		if (file.is_open())
			file.write(reinterpret_cast<const char*>(std::data(output)), std::size(output));
		else
			//todo throw instead
			LOGERROR("Failed to open file for writing: " + p.generic_string());
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
