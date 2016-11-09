#ifndef HADES_ARCHIVE_HPP
#define HADES_ARCHIVE_HPP

#include <string>

namespace hades
{
	namespace zip
	{
		struct archive;

		//open_archive;

		//returns raw data
		//read_file_from_archive;

		//returns streamable data
		//stream_file_from_archive;

		//returns a string containing the files contents
		//read_text_file_from_archive;

		//close_archive;

		//ditermines if a file within an archive exists
		bool file_exists(archive* a, std::string path);

		//compress_directory
		bool compress_directory(std::string path);
		//uncompress archive
		bool uncompress_archive(std::string path);
	}
}

#endif // hades_data_hpp
