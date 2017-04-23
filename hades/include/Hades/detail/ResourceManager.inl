#include <cassert>

#include "SFML/Graphics/Texture.hpp"

#include "Hades/StandardPaths.hpp"

namespace hades
{
	template<typename T>
	detail::ResourceHolder<T> &GetHolder(ResourceManager::ResourceHolderMap &map)
	{
		auto &h = map[typeid(T)];
		if(h == nullptr)
			h = std::make_unique<detail::DefinedHolder<T>>();

		assert(dynamic_cast<detail::DefinedHolder<T>*>(&*h) == static_cast<detail::DefinedHolder<T>*>(&*h));
		return static_cast<detail::DefinedHolder<T>*>(&*h)->resourceHolder;
	}

	//============ResourceManager===============
	template<class T>
	std::shared_ptr<T> ResourceManager::loadFromCache(const std::string &path)
	{
		auto &map = GetHolder<T>(_resourceHolders);
		return map[path];
	}

	template<class T>
	void ResourceManager::recordToCache(const std::string &path, std::shared_ptr<T> &resource)
	{
		auto &map = GetHolder<T>(_resourceHolders);
		map[path] = resource;
	}

	template<class T> 
	void ResourceManager::ConfigureResource(std::shared_ptr<T> &r, const std::string &path) {}

	template<> 
	inline void ResourceManager::ConfigureResource<sf::Texture>(std::shared_ptr<sf::Texture> &r, const std::string &path)
	{
		r->setRepeated(true);
	}

	template<>
	inline void ResourceManager::ConfigureResource<TextFile>(std::shared_ptr<TextFile> &r, const std::string &path)
	{
		r->SetName(path);
		assert(r->GetName() == path);
	}

	template<class T>
	std::shared_ptr<T> ResourceManager::LoadFromFile(const std::string &dir, const std::string &path)
	{
		//SFINAE: static assert if T doesnt impliment loadFromFile

		std::shared_ptr<T> r = std::make_shared<T>();
		//Test for file existance before trying to load.
		if (fileExists(dir + path) && r->loadFromFile(dir + path))
			return r;
		else
			return nullptr;
	}

	template<class T>
	std::shared_ptr<T> ResourceManager::LoadFromMemory(const std::string &dir, const std::string &path)
	{
		std::shared_ptr<T> r;
		//check if file is there
		//load file
		//r = _resources->acquire(thor::Resources::fromMemory<T>(path));
		if(r)
			return r;

		return nullptr;
	}

	template<class T>
	std::shared_ptr<T> ResourceManager::LoadFromArchive(const std::string &dir, const std::string &path)
	{
		//console->echo("Archive loading not implemented", Console::ERROR);
		return nullptr;
		std::shared_ptr<T> r;
		//check if file is there
		//Archive(dir + ".sack");
		//load file
		r = LoadFromMemory<T>(dir, path);
		if(r)
			return r;
	}

	template<class T>
	std::shared_ptr<T> ResourceManager::LoadResource(const std::string &dir, const std::string &path)
	{
		//load from steam workshop?
		std::shared_ptr<T> r;



		//==load from user folder==
		auto UserModDir = getUserCustomFileDirectory();
		//load as folder path
		r = LoadFromFile<T>(UserModDir + dir, path);
		if(r)
			return r;
		//load as archive
		r = LoadFromArchive<T>(UserModDir + dir, path);
		if(r)
			return r;

		//==load from game directory==
		//path as folder
		r = LoadFromFile<T>(dir, path);
		if(r)
			return r;
		//path in archive
		//load archive, find file, then load from memory
		r = LoadFromArchive<T>(dir, path);
		if(r)
			return r;
		//load from memory
		//TODO: Support Archive :S}

		

		//catch all failure state
		r = nullptr;
		return r;
	}

	template<class T>
	std::shared_ptr<const T> ResourceManager::getResource(const std::string &relativePath)
	{
		auto normalisedPath = normalisePath(relativePath);

		std::lock_guard<std::mutex> resourceLock(_resourceMutex);
		std::shared_ptr<T> r = loadFromCache<T>(normalisedPath);

		if(!r)
		{
			//load from resources, we do it in revserse because we want the most recently added directories first.
			for(auto i = path.rbegin(); i != path.rend(); ++i)
			{
				r = LoadResource<T>(*i, normalisedPath);
				if (r)
				{
					ConfigureResource(r, normalisedPath);
					recordToCache(normalisedPath, r);
					break;
				}
			}
		}

		if(!r)
		{
			LOGERROR("Could not find file: \"" + relativePath + "\"");
			return nullptr;
		}

		return r;
	}
}