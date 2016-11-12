#include "Hades/ResourceManager.hpp"

#include <algorithm>
#include <filesystem>
#include <vector>

#include "Hades/StandardPaths.hpp"

namespace std {
	using namespace std::experimental;
}

namespace hades
{
	bool TextFile::loadFromFile(std::string filepath)
	{
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			std::string contents;
			in.seekg(0, std::ios::end);
			contents.resize(static_cast<unsigned int>(in.tellg()));
			in.seekg(0, std::ios::beg);
			in.read(&contents[0], contents.size());
			in.close();

			data = contents;
			name = filepath;
			return true;
		}

		return false;
	}

	std::string TextFile::GetDirectory() const
	{
		auto path = std::filesystem::path(name);
		return path.parent_path().string() + "/";
	}

	typedef std::vector<std::string>  FileList;
	void listFilesInZip(const std::string &dir, const std::string &subdir, FileList &files);
	void listFilesInDir(const std::string &dir, FileList &files);

	//================ResourceManager=============
	void ResourceManager::appendSystemPath(const std::string &dir) 
	{ 
		#ifdef _DEBUG // So that game files can be accessed without rearanging the build directory.
			path.push_back("./../../game/" + dir);
		#endif // DEBUG

		path.push_back(dir);
	}

	void ResourceManager::clear()
	{
		std::lock_guard<std::mutex> resourceLock(_resourceMutex);
		_resourceHolders.clear();
	}

	bool ResourceManager::fileExists(const std::string &relativeFilePath) const
	{
		FileList filesInDir;

		//+1 to include the slash in the directory string, and exclude it from the filename
		auto splitPos = relativeFilePath.find_last_of('/') + 1;
		std::string directory(relativeFilePath.begin(), relativeFilePath.begin() + splitPos);
		
		//bail if the directory doesn't exist
		//it obviously cannot cantain the file
		if (!std::filesystem::is_directory(directory))
			return false;

		listFilesInDir(directory, filesInDir);

		//if it list is empty the file cannot be there
		if (filesInDir.empty())
			return false;

		//otherwise check if the specified file is present
		//std::string filename(relativeFilePath.begin() + splitPos, relativeFilePath.end());
		if (std::find(filesInDir.begin(), filesInDir.end(), relativeFilePath) != filesInDir.end())
			return true;

		return false;
	}

	std::set<std::string> ResourceManager::listFilesInDirectory(const std::string &relativeDirectory) const
	{
		std::set<std::string> filelist;
		auto userDir = getUserCustomFileDirectory();

		for(auto i = path.rbegin(); i != path.rend(); ++i)
		{
			
			std::vector<std::string> list;
			//user dir
			if (std::filesystem::is_directory(userDir + *i + relativeDirectory))
				listFilesInDir(userDir + *i + relativeDirectory, list);
			//user archive
			if (std::filesystem::exists(userDir + *i + HADES_ARCHIVE_EXT))
				listFilesInZip(userDir + *i + HADES_ARCHIVE_EXT, relativeDirectory, list);
			
			//game dir
			if (std::filesystem::is_directory(*i + relativeDirectory))
				listFilesInDir(*i + relativeDirectory, list);
			//game archive
			if (std::filesystem::exists(userDir + *i + HADES_ARCHIVE_EXT))
				listFilesInZip(*i + HADES_ARCHIVE_EXT, relativeDirectory, list);

			//remove resource dir prefixes from path names
			for (auto &s : list)
			{
				auto pos = s.find_first_of(*i);
				s.erase(pos, pos + i->length());
				filelist.insert(s);
			}
		}

		return filelist;
	}

	void listFilesInZip(const std::string &dir, const std::string &subdir, FileList &files)
	{
		//TODO: zip support
		return;
	}

	void listFilesInDir(const std::string &dir, FileList &files)
	{
		//TODO: catch exception here if dir doesn't exist as a directory.
		auto directory = std::filesystem::directory_iterator(dir);

		while(directory != std::filesystem::directory_iterator())
		{
			if(!std::filesystem::is_directory(directory->path()))
				files.push_back(normalisePath(directory->path().string()));

			++directory;
		}
	}

	//substring replacement func from:
	//https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
	//by user 'minastaros'
	void replaceAll(std::string& source, const std::string& from, const std::string& to)
	{
		std::string newString;
		newString.reserve(source.length());  // avoids a few memory allocations

		std::string::size_type lastPos = 0;
		std::string::size_type findPos;

		while (std::string::npos != (findPos = source.find(from, lastPos)))
		{
			newString.append(source, lastPos, findPos - lastPos);
			newString += to;
			lastPos = findPos + from.length();
		}

		// Care for the rest after last occurrence
		newString += source.substr(lastPos);

		source.swap(newString);
	}

	std::string normalisePath(const std::string &relativeFilePath)
	{
		auto path = relativeFilePath;

		//replace all occurances of "\\"(the windows escaped slash) with '/'
		replaceAll(path, "\\", "/");

		return path;
	}
}