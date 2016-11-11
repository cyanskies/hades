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
				FILE_ALREADY_OPEN
			};

			archive_exception(const char* what, error_code code);

			error_code code() const;
			const char* what() const;
		private:
			std::string _what;
			error_code _code;
		};

		class archive_stream : public sf::InputStream
		{
		public:
			archive_stream(std::string archive);

			virtual ~archive_stream();

			bool open(std::string filename);
			sf::Int64 read(void* data, sf::Int64 size);
			sf::Int64 seek(sf::Int64 position);
			sf::Int64 tell();
			sf::Int64 getSize();
		private:
			std::string _archivePath, _fileName;
			bool _fileOpen;
			unarchive _archive;
		};

		using archive_stream_ptr = std::shared_ptr<archive_stream>;
		using buffer = std::vector<types::uint8>;

		//open a close unzip archives
		unarchive open_archive(std::string path);
		void close_archive(unarchive f);

		//returns raw data
		buffer read_file_from_archive(std::string archive, std::string path);
		buffer read_file_from_archive(unarchive, std::string path);

		//returns a file read as a string
		std::string read_text_from_archive(std::string archive, std::string path);
		std::string read_text_from_archive(unarchive, std::string path);

		//returns streamable data
		archive_stream_ptr stream_file_from_archive(std::string archive, std::string path);

		//ditermines if a file within an archive exists
		bool file_exists(std::string archive, std::string path);
		bool file_exists(unarchive, std::string path);

		//compress_directory
		bool compress_directory(std::string path);
		//uncompress archive
		bool uncompress_archive(std::string path);
	}
}

#endif // hades_data_hpp
