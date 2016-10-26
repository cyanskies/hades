#ifndef HADES_DATAMANAGER_HPP
#define HADES_DATAMANAGER_HPP

#include <unordered_set>
#include <string>

#include "Types.hpp"

#include "detail/DataManager.inl"

namespace hades
{

	using UniqueId = UniqueId_t<hades::types::uint64>;

	class CustomType;

	class DataManager
	{
	public:
		//application registers the custom resource types
		register_custom_resource();
		//application registers the handler for the [GAME] section
		register_game();
		//loads folder or archive as the main game
		load_game();
		//loads a folder or archive as a mod
		load_mod();
		//sets a custom type
		setCustomType(UniqueId, std::string source);
		CustomType *getCustomType(UniqueId);

		//register a new uid
		setUid(std::string name);
		//convert string to uid
		UniqueId getUid(std::string name);

	private:

		//list of used names
		std::unordered_set<std::string> _names;
	};
}

#endif // hades_data_hpp
