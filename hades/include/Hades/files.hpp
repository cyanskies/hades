#ifndef HADES_FILES_HPP
#define HADES_FILES_HPP

#include "SFML/System/FileInputStream.hpp"
#include "SFML/System/InputStream.hpp"

#include "Hades/archive.hpp"

namespace hades
{
	namespace files
	{
		class FileStream;

		std::string as_string(const std::string &modPath, const std::string &fileName);
		buffer as_raw(const std::string &modPath, const std::string &fileName);
		FileStream make_stream(const std::string &modPath, const std::string &fileName);

		class FileStream : public sf::InputStream
		{
		public:
			FileStream() {}
			FileStream(const std::string &modPath, const std::string &fileName);
			
			~FileStream();

			FileStream(FileStream&&);
			FileStream(const FileStream&) = delete;
			FileStream &operator=(FileStream&&);
			FileStream &operator=(const FileStream&) = delete;

			void open(const std::string &modPath, const std::string &fileName);

			bool is_open() const
			{
				return _open;
			}

			sf::Int64 read(void* data, sf::Int64 size);
			sf::Int64 seek(sf::Int64 position);
			sf::Int64 tell();
			sf::Int64 getSize();

		private:
			bool archive = false, file = false, _open = false;
			std::string _mod_path, _file_path;

			union 
			{
				sf::FileInputStream _fileStream;
				zip::archive_stream _archiveStream;
			};
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

		std::vector<types::string> ListFilesInDirectory(types::string dir_path);
	}
}

#endif // hades_files_hpp
