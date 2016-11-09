#ifndef HADES_DATAMANAGER_HPP
#define HADES_DATAMANAGER_HPP

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "type_erasure.hpp"
#include "Types.hpp"

namespace hades
{
	namespace data
	{
		template<typename id_type>
		class UniqueId_t
		{
		public:
			using type = id_type;

			UniqueId_t(id_type value) : _value(value) {}

			bool operator==(const UniqueId_t& rhs) const
			{
				return _value == rhs._value;
			}

			type get() const { return _value; }
		private:
			type _value;
		};

		using UniqueId = UniqueId_t<hades::types::uint64>;

	}
}

namespace std {
	template <> struct hash<hades::data::UniqueId>
	{
		size_t operator()(const hades::data::UniqueId& key) const
		{
			std::hash<hades::data::UniqueId::type> h;
			return h(key.get());
		}
	};
}

namespace hades
{
	namespace data
	{
		class resource_base : public type_erasure::type_erased_base
		{
		public:
			void load();

			UniqueId getUid() const;
		private:
			//incremented if the data is reloaded.
			UniqueId _id;
			types::uint8 _generation = 1;
			//store loader function;
		};

		template<class T>
		class resource : public type_erasure::type_erased<T>
		{
		public:
			resource(UniqueId id) : _id(id) {}
		};

		using resource_ptr = std::unique_ptr<resource_base>;

		template<class T>
		class Resource
		{
		public:
			bool is_ready() const;
			T const * const get() const;

		private:
			data::resource<T> *_r;
			types::uint8 _generation = 0;
		};

		class data_manager
		{
		public:

			using parserFunc = std::function<void(void)>;
			using loaderFunc = std::function<void(resource_ptr*)>;

			virtual ~data_manager();
			//application registers the custom resource types
			//parser must convert yaml into a resource manifest object
			void register_resource_type(std::string name, parserFunc parser);
			//loader reads manifest and loads data from disk
			void register_resource_type(std::string name, parserFunc parser, loaderFunc loader);

			//game is the name of a folder or archive containing a game.yaml file
			void load_game(std::string game);
			//mod is the name of a folder or archive containing a mod.yaml file
			void add_mod(std::string mod);

			template<class T>
			Resource<T> get(UniqueId);

			//register a new uid
			UniqueId createUid(std::string name);
			//convert string to uid
			UniqueId getUid(std::string name);

		private:
			//==parsing and loading data==
			std::unordered_map<std::string, parserFunc> _resourceParsers;

			//==stored resource data==
			//list of used names
			std::unordered_set<std::string> _names;
			//map of names to Uids
			std::unordered_map<std::string, UniqueId> _ids;
			//map of uids to resources
			property_bag<UniqueId, resource_base> _resources;
			//list of unloaded resources
			std::vector<resource_ptr*> _loadQueue;
		};
	}

	//default hades datamanager
	//registers that following resource types:
	//texture
	//sound
	//document
	//string store
	class DataManager : public data::data_manager
	{
	public:
	};
}

#include "detail/DataManager.inl"

#endif // hades_data_hpp
