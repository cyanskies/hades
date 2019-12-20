#ifndef HADES_FILES_HPP
#define HADES_FILES_HPP

#include <cassert>
#include <filesystem>
#include <fstream>
#include <variant>
#include <vector>

#include "Hades/archive.hpp"
#include "hades/exceptions.hpp"

//TODO: use filesystem path instead of string

namespace hades::files
{
	//hades input file stream
	//defers to izfsteam for compressed files
	class ifstream
	{
	public:
		using char_t = std::byte;
		using stream_t = std::ifstream;
		using pos_type = stream_t::traits_type::pos_type;
		using off_type = stream_t::traits_type::off_type;

		ifstream() = default;
		explicit ifstream(const std::filesystem::path&);

		ifstream(const ifstream&) = delete;
		ifstream& operator=(const ifstream&) = delete;

		//std::ifstream is not noexcept move
		ifstream(ifstream&&) = default;
		ifstream& operator=(ifstream&&) = default;

		void open(const std::filesystem::path&);
		void close() noexcept;
		bool is_open() const noexcept;

		ifstream& read(char_t*, std::size_t);
		std::streamsize gcount() const;
		pos_type tellg();

		bool eof() const;

		void seekg(pos_type);
		void seekg(off_type, std::ios_base::seekdir);

	private:
		std::variant<stream_t, zip::izfstream> _stream{};
	};

	//open streams to files, saves and configs
	ifstream stream_file(const std::filesystem::path&);
	ifstream stream_save(const std::filesystem::path&);
	ifstream stream_config(const std::filesystem::path&);

	//reads file at path as a string
	//will attempt to load from user_custom_file_directory first
	string read_file(const std::filesystem::path& file_path);
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
		using stream_t = files::ifstream;
		using pos_type = stream_t::pos_type;
		using off_type = stream_t::off_type;

		irfstream() = default;
		irfstream(const std::filesystem::path& mod, const std::filesystem::path& file);

		irfstream(const irfstream&) = delete;
		irfstream& operator=(const irfstream&) = delete;

		irfstream(irfstream&&) = default;
		irfstream& operator=(irfstream&&) = default;

		void open(const std::filesystem::path& mod, const std::filesystem::path& file);
		void close() noexcept;
		bool is_open() const noexcept;

		irfstream& read(char_t*, std::size_t);
		std::streamsize gcount() const;
		pos_type tellg();

		bool eof() const;

		void seekg(pos_type);
		void seekg(off_type, std::ios_base::seekdir);

		std::size_t size();

	private:
		bool _try_open_from_dir(const std::filesystem::path& dir, const std::filesystem::path& mod, const std::filesystem::path& file);
		bool _try_open_file(const std::filesystem::path& file);
		bool _try_open_archive(const std::filesystem::path& archive, const std::filesystem::path& file);

		std::variant<files::ifstream, zip::iafstream> _stream{};
	};
}

namespace hades::files
{
	//reads game files either from directories or mod archives
	//throws file_exception
	string as_string(std::string_view modPath, std::string_view fileName);
	//throws file_exception
	buffer as_raw(std::string_view modPath, std::string_view fileName);

	//writes file_contents to the file at path, will place UserCustomFileDirectory before path
	//throws file_exception
	void write_file(const std::filesystem::path& path, std::string_view file_contents);
	//same as above, calls UserSaveDirectory instead
	void write_save(std::string_view);
	//as above, calls UserConfigDir instead
	void write_config(std::string_view);

	std::vector<types::string> ListFilesInDirectory(std::string_view dir_path);
}

#endif // hades_files_hpp
