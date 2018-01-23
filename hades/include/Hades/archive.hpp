#ifndef HADES_ARCHIVE_HPP
#define HADES_ARCHIVE_HPP

#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "SFML/System/InputStream.hpp"

#include "Types.hpp"

namespace hades
{
	//buffer of bytes
	using buffer = std::vector<types::uint8>;

	namespace zip
	{
		using unarchive = void*;

		class archive_exception : public std::runtime_error
		{
		public:
			enum class error_code
			{
				FILE_OPEN,
				FILE_READ,
				FILE_WRITE,
				FILE_CLOSE,
				FILE_ALREADY_OPEN,
				FILE_NOT_OPEN,
				FILE_NOT_FOUND
			};

			archive_exception(const char* what, error_code code);

			error_code code() const;
		private:
			error_code _code;
		};

		class archive_stream : public sf::InputStream
		{
		public:
			archive_stream(types::string archive);
			archive_stream(archive_stream&&);
			archive_stream(const archive_stream&) = delete;
			archive_stream &operator=(archive_stream&&);
			archive_stream &operator=(const archive_stream&) = delete;

			virtual ~archive_stream();

			bool open(types::string filename);
			bool is_open() const;
			sf::Int64 read(void* data, sf::Int64 size);
			sf::Int64 seek(sf::Int64 position);
			sf::Int64 tell();
			sf::Int64 getSize();
		private:
			void _close();

			types::string _fileName;
			bool _fileOpen;
			unarchive _archive;
		};

		//returns raw data
		buffer read_file_from_archive(types::string archive, types::string path);

		//returns a file read as a string
		types::string read_text_from_archive(types::string archive, types::string path);

		//returns streamable data
		archive_stream stream_file_from_archive(types::string archive, types::string path);

		//ditermines if a file within an archive exists
		bool file_exists(types::string archive, types::string path);

		//returns all files in the archive within the dir_path directory, can continue recursively
		std::vector<types::string> list_files_in_archive(types::string archive, types::string dir_path, bool recursive = false);

		//compress_directory
		//path must be a directory
		//will create a zip in the parent directory with the same name
		void compress_directory(types::string path);
		//uncompress archive
		void uncompress_archive(types::string path);
	}
}

#endif // hades_data_hpp
