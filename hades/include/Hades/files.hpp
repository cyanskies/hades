#ifndef HADES_FILES_HPP
#define HADES_FILES_HPP

#include <cassert>
#include <variant>
#include <vector>

#include "SFML/System/FileInputStream.hpp"
#include "SFML/System/InputStream.hpp"

#include "Hades/archive.hpp"

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

		//throws file_exception
		types::string as_string(std::string_view modPath, std::string_view fileName);
		//throws file_exception
		buffer as_raw(std::string_view modPath, std::string_view fileName);
		//throws file_exception
		ResourceStream make_stream(std::string_view modPath, std::string_view fileName);
		//as above, but checks the usersSaveDirectoryinstead
		ResourceStream make_save_stream(std::string_view fileName);
		//as above, userConfigDir
		ResourceStream make_config_stream(std::string_view fileName);

		//writes file_contents to the file at path, will place UserCustomFileDirectory before path
		//throws file_exception
		void write_file(std::string_view path, std::string_view file_contents);
		//same as above, calls UserSaveDirectory instead
		void write_save();
		//as above, calls UserConfigDir instead
		void write_config();

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
			void open(std::string_view modPath, std::string_view fileName);

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

			file_exception(const char* what, error_code code) : std::runtime_error(what), _code(code) {}

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
