#ifndef HADES_FILES_HPP
#define HADES_FILES_HPP

#include <cassert>
#include <variant>
#include <vector>

#include "SFML/System/FileInputStream.hpp"
#include "SFML/System/InputStream.hpp"

#include "Hades/archive.hpp"

//TODO: use filesystem path instead of string

namespace hades
{
	namespace files
	{
		namespace detail
		{
			//This wrapper over sf::FileInputStream to allow move semantics
			class FileStream
			{
			public:
				FileStream() : _fstream(std::make_unique<sf::FileInputStream>())
				{}

				~FileStream() = default;

				FileStream(const FileStream&) = delete;
				FileStream &operator=(const FileStream&) = delete;

				FileStream(FileStream&&) = default;
				FileStream &operator=(FileStream&&) = default;

				bool open(const std::string& filename) 
				{
					assert(_fstream);
					return _fstream->open(filename);
				}

				sf::Int64 read(void* data, sf::Int64 size)
				{
					assert(_fstream);
					return _fstream->read(data, size);
				}

				sf::Int64 seek(sf::Int64 position)
				{
					assert(_fstream);
					return _fstream->seek(position);
				}

				sf::Int64 tell()
				{
					assert(_fstream);
					return _fstream->tell();
				}

				sf::Int64 getSize()
				{
					assert(_fstream);
					return _fstream->getSize();
				}

			private:
				std::unique_ptr<sf::FileInputStream> _fstream;
			};
		}

		class ResourceStream;

		//reads game files either from directories or mod archives
		//throws file_exception
		types::string as_string(std::string_view modPath, std::string_view fileName);
		//throws file_exception
		buffer as_raw(std::string_view modPath, std::string_view fileName);

		//reads file at path as a string
		//will attempt to load from user_custom_file_directory first
		string read_file(std::string_view file_path);
		// as above, but checks the usersSaveDirectoryinstead
		//reads save files and config files
		types::string read_save(std::string_view file_name);
		//as above, userConfigDir
		types::string read_config(std::string_view file_name);

		//throws file_exception
		ResourceStream make_stream(std::string_view modPath, std::string_view fileName);
		//writes file_contents to the file at path, will place UserCustomFileDirectory before path
		//throws file_exception
		void write_file(std::string_view path, std::string_view file_contents);
		//same as above, calls UserSaveDirectory instead
		void write_save(std::string_view);
		//as above, calls UserConfigDir instead
		void write_config(std::string_view);

		//this is a helper class for streaming resources that might be inside
		// an archive
		class ResourceStream final : public sf::InputStream
		{
		public:
			ResourceStream() = default;
			//throws file_exception
			ResourceStream(std::string_view modPath, std::string_view fileName);
			
			ResourceStream(ResourceStream&&) = default;
			ResourceStream(const ResourceStream&) = delete;
			ResourceStream &operator=(ResourceStream&&) = default;
			ResourceStream &operator=(const ResourceStream&) = delete;

			//throws file_exception
			bool open(std::string_view modPath, std::string_view fileName);

			bool is_open() const
			{
				return _open;
			}

			//these all throw std::logic_error
			sf::Int64 read(void* data, sf::Int64 size);
			sf::Int64 seek(sf::Int64 position);
			sf::Int64 tell();
			sf::Int64 getSize();

		private:
			bool _open = false;
			types::string _mod_path, _file_path;

			using File = detail::FileStream;
			using Archive = zip::archive_stream;
			using StreamVariant = std::variant<File, Archive>;
			StreamVariant _stream;
		};

		class file_exception : public std::runtime_error
		{
		public:
			enum class error_code
			{
				UNREADABLE_FILE,
				FILE_NOT_FOUND,
				PATH_NOT_FOUND,
				ARCHIVE_INVALID,
				FILE_NOT_OPEN,
				FILENAME_INVALID,
				PERMISSION_DENIED
			};

			file_exception(const char* what, error_code code) : std::runtime_error{ what }, _code{ code } {}
			file_exception(std::string what, error_code code) : std::runtime_error{ what }, _code{ code } {}

			error_code code() const
			{
				return _code;
			}

		private:
			error_code _code;
		};

		std::vector<types::string> ListFilesInDirectory(std::string_view dir_path);
	}
}

#endif // hades_files_hpp
