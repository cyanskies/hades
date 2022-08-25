#ifndef HADES_FILES_HPP
#define HADES_FILES_HPP

#include <cassert>
#include <filesystem>
#include <fstream>
#include <variant>
#include <vector>

#include "hades/archive.hpp"
#include "hades/exceptions.hpp"

namespace hades::data
{
	struct mod;
}

namespace hades::files
{
	//hades input file stream
	//defers to izfstream for compressed files
	using ifstream = zip::izfstream;
	static_assert(std::is_move_constructible_v<ifstream>);

	// compresses output if file_deflate flag is set
	class ofstream : public std::ostream
	{
	public:
		ofstream() noexcept;
		explicit ofstream(const std::filesystem::path&);

		ofstream(ofstream&&) noexcept;
		ofstream& operator=(ofstream&&) noexcept;

		void open(const std::filesystem::path&);
		void close() noexcept;

		bool is_open() const noexcept;

	private:
		std::variant<zip::out_compressed_filebuf, std::filebuf> _stream;
	};

	//TODO: check error paths/ throw file not found?
	//open streams to files, saves and configs
	ifstream stream_file(const std::filesystem::path&);
	ifstream stream_save(const std::filesystem::path&);
	ifstream stream_config(const std::filesystem::path&);

	//TODO: check error paths/ throw file not found?
	//reads file at path as a string
	//will attempt to load from user_custom_file_directory first
	string read_file(const std::filesystem::path& file_path);
	buffer raw_file(const std::filesystem::path& file_path); // TODO: remove? including all its leaf functions
	// as above, but checks the usersSaveDirectoryinstead
	//reads save files and config files
	string read_save(const std::filesystem::path& file_name);
	//as above, userConfigDir
	string read_config(const std::filesystem::path& file_name);
}

namespace hades
{
	//in resource file stream
	//loads using mod override policy
	//first selects overrides in the users custom dir
	//then the mods in user custom dir
	//then overrides in the game dir
	// then mods in the game dir
	class irfstream
	{
	public:
		using char_t = std::byte;
		using char_type = char_t;
		using stream_t = files::ifstream;
		using pos_type = stream_t::pos_type;
		using off_type = stream_t::off_type;

		irfstream() = default;
		irfstream(const std::filesystem::path& mod, const std::filesystem::path& file)
		{
			open(mod, file);
			return;
		}

		irfstream(irfstream&&) noexcept(std::is_nothrow_move_constructible_v<stream_t>
			&& std::is_nothrow_move_constructible_v<zip::iafstream>) = default;
		irfstream& operator=(irfstream&&) noexcept(std::is_nothrow_move_assignable_v<stream_t>
			&& std::is_nothrow_move_assignable_v<zip::iafstream>) = default;

		void open(const std::filesystem::path& mod, const std::filesystem::path& file);
		void close() noexcept;
		bool is_open() const noexcept;

		std::filesystem::path mod_path() const
		{
			return _mod_path;
		}

		std::filesystem::path path() const
		{
			return _rel_path;
		}

		irfstream& read(char_t*, std::size_t);
		std::streamsize gcount() const;
		pos_type tellg();

		bool eof() const;

		void seekg(pos_type);
		void seekg(off_type, std::ios_base::seekdir);

		std::size_t size();

	private:
		bool _try_open_from_dir(const std::filesystem::path&, const std::filesystem::path&, const std::filesystem::path&);
		bool _try_open_file(const std::filesystem::path&, const std::filesystem::path&);
		bool _try_open_archive(std::filesystem::path, const std::filesystem::path&);

		std::variant<files::ifstream, zip::iafstream> _stream{};
		std::filesystem::path _mod_path;
		std::filesystem::path _rel_path;
	};

	static_assert(std::is_move_constructible_v<irfstream>);
}

namespace hades::files
{
	//TODO: check error paths/ throw file not found?
	irfstream stream_resource(const data::mod&, const std::filesystem::path& path);
	buffer raw_resource(const data::mod&, const std::filesystem::path& path);
	string read_resource(const data::mod&, const std::filesystem::path& path);

	//writes file_contents to the file at path,
	//will place UserCustomFileDirectory before path if portable flag isnt set
	//throws file_exception
	// overwrites any existing file
	ofstream write_file(const std::filesystem::path&);
	void write_file(const std::filesystem::path& path, std::string_view file_contents);
	// uncompressed stream to a file in UserCustomFileDirectory
	std::ofstream append_file_uncompressed(const std::filesystem::path& path);
	//same as above, calls UserSaveDirectory instead
	void write_save(std::string_view);
	//as above, calls UserConfigDir instead
	void write_config(std::string_view);


	std::vector<types::string> ListFilesInDirectory(std::string_view dir_path);
}

#endif // hades_files_hpp
