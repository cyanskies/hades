#ifndef HADES_RESOURCE_HPP
#define HADES_RESOURCE_HPP

#include <fstream>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include <set>
#include <typeindex>
#include <vector>

#include "Hades/Logging.hpp"

namespace hades
{ 
	static const auto HADES_ARCHIVE_EXT = ".zip";

	class TextFile
	{
	private:
		std::string data;
		std::string name;

	public:
		bool loadFromFile(std::string filepath);

		std::string GetDirectory() const;
		std::string GetData() const { return data; }
		std::string GetName() const { return name; }
		void SetName(const std::string &filename) { name = filename; }
	};

	namespace detail
	{
		struct AnonymousHolder
		{
			virtual ~AnonymousHolder() = default;
		};

		template<typename T>
		using ResourceHolder = std::unordered_map<std::string, std::shared_ptr<T>>;

		template<typename T>
		struct DefinedHolder : public AnonymousHolder
		{
			ResourceHolder<T>  resourceHolder;
		};
	}

	class ResourceManager
	{
	public:
		typedef std::unordered_map<std::type_index, std::unique_ptr<detail::AnonymousHolder>> ResourceHolderMap;

	private:
		std::mutex _resourceMutex; //guarrentees the multithreaded consistancy of the manager by only allow one resource to be loaded at a time
	
		ResourceHolderMap _resourceHolders;
		
		std::vector<std::string> path;
		
		template<class T>
		void ConfigureResource(std::shared_ptr<T> &resource, const std::string &path);

		template<class T>
		std::shared_ptr<T> loadFromCache(const std::string &path);

		template<class T>
		void recordToCache(const std::string &path, std::shared_ptr<T> &resource);
		
		template<class T>
		std::shared_ptr<T> LoadFromFile(const std::string &dir, const std::string &path);

		//LoadFromMemory is a helper method used by LoadFromArchive
		template<class T>
		std::shared_ptr<T> LoadFromMemory(const std::string &dir, const std::string &path);

		template<class T>
		std::shared_ptr<T> LoadFromArchive(const std::string &dir, const std::string &path);

		template<class T>
		std::shared_ptr<T> LoadResource(const std::string &dir, const std::string &path);

		//Returns true if a file with the specified name exists
		bool fileExists(const std::string &relativeFilePath) const;

	public:
		//Paths are searched in the order they were added.
		//Standard paths are:
		//Usr common folder(./common/): an per user folder(C:/users/myname/appdata/gamename/common/) that overrides files User common archive, and by extension all other common folders and archives.
		//Usr common archive(./common/): an per user archive(C:/users/myname/appdata/gamename/common/) that overrides files in the apps folders and archives.
		//App common folder(./common/): a folder in the app directory that can be used to override files in the app provided common archive.
		//App common archive(./common/): minimum resources for loading into the menu and playing a demo version of the game.
		//
		//A simmilar override structure exists for the other standard and custom paths, but won't be duplicated here for simplicities sake.
		//App game archive(./game/): the remaining content used by the base game.
		//App customX(./CUSTOM/): app defined additional archive names for dlc content or content that can be enabled or disabled by options.
		//
		// Content purity can be used to allow content to be loaded only from standard App archives.
		void appendSystemPath(const std::string &dir);
		/*void setUserPath(const std::string &dir) { userPath = dir; }*/

		template<class T>
		std::shared_ptr<const T> getResource(const std::string &relativePath);

		//clears the resource cache, any resources not being used will be cleared.
		//any resources still being used will no longer be tracked.
		void clear();


		// Lists all the files in the specified directory.
		// This includes copies of that directory in each of the standard and custom paths.
		// returns all the files, with the directory path prepended onto them.
		std::set<std::string> listFilesInDirectory(const std::string &relativeDirectory) const;

	}; // class ResourceManager

	//replaces the tokens in a filepath with the more accepted posix equivalents
	std::string normalisePath(const std::string &relativeFilePath);
}//hades

#include "detail/ResourceManager.inl"

#endif // hades_resource_hpp
