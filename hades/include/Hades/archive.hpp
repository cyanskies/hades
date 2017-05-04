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

		class archive_exception : public std::exception
		{
		public:
			enum class error_code
			{
				FILE_OPEN,
				FILE_READ,
				FILE_CLOSE,
				FILE_ALREADY_OPEN,
				FILE_NOT_OPEN
			};

			archive_exception(const char* what, error_code code);

			error_code code() const;
		private:
			error_code _code;
		};

		class archive_stream : public sf::InputStream
		{
		public:
			archive_stream(std::string archive);
			archive_stream(archive_stream&&);
			archive_stream(const archive_stream&) = delete;
			archive_stream &operator=(archive_stream&&);
			archive_stream &operator=(const archive_stream&) = delete;

			virtual ~archive_stream();

			bool open(std::string filename);
			bool is_open() const;
			sf::Int64 read(void* data, sf::Int64 size);
			sf::Int64 seek(sf::Int64 position);
			sf::Int64 tell();
			sf::Int64 getSize();
		private:
			std::string _fileName;
			bool _fileOpen;
			unarchive _archive;
		};
		
		//returns raw data
		buffer read_file_from_archive(std::string archive, std::string path);

		//returns a file read as a string
		std::string read_text_from_archive(std::string archive, std::string path);

		//returns streamable data
		archive_stream stream_file_from_archive(std::string archive, std::string path);

		//ditermines if a file within an archive exists
		bool file_exists(std::string archive, std::string path);

		//compress_directory
		//path must be a directory
		//will create a zip in the parent directory with the same name
		void compress_directory(std::string path);
		//uncompress archive
		void uncompress_archive(std::string path);
	}
}

#endif // hades_data_hpp
