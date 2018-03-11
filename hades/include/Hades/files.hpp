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

		types::string as_string(std::string_view modPath, std::string_view fileName);
		buffer as_raw(std::string_view modPath, std::string_view fileName);
		ResourceStream make_stream(std::string_view modPath, std::string_view fileName);

		class ResourceStream final : public sf::InputStream
		{
		public:
			ResourceStream() = default;
			ResourceStream(std::string_view modPath, std::string_view fileName);
			
			ResourceStream(ResourceStream&&) = default;
			ResourceStream(const ResourceStream&) = delete;
			ResourceStream &operator=(ResourceStream&&) = default;
			ResourceStream &operator=(const ResourceStream&) = delete;

			void open(std::string_view modPath, std::string_view fileName);

			bool is_open() const
			{
				return _open;
			}

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
				FILE_NOT_OPEN
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
