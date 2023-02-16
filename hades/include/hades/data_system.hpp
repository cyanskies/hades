#ifndef HADES_DATAMANAGER_HPP
#define HADES_DATAMANAGER_HPP

#include <iosfwd>
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "hades/data.hpp"
#include "hades/parser.hpp"
#include "hades/resource_base.hpp"
#include "hades/types.hpp"

namespace hades::data
{
	class data_system final : public data_manager
	{
	public:
		data_system();

		//application registers the custom resource types
		//parser must convert yaml into a resource manifest object
		void register_type(std::string_view name, resources::parser_func parser) override;

		//game is the name of a folder or archive containing a game.yaml file
		void load_game(std::string_view game);
		//mod is the name of a folder or archive containing a mod.yaml file
		// throws parser_exception
		void add_mod(std::string_view mod, bool autoLoad = false, std::filesystem::path name = "./mod.yaml");

		bool try_load_mod_impl(std::string_view) override;

		//returns true if the mod or game with that name has been parsed
		//note: this uses the mods self identified name, not the archive or folder name(as this can change in debug mode)
		bool loaded(std::string_view mod);

		//reread data from all mods, starting with game, and doing them all in order
		void reparse();

		//adds all objects into the load queue
		//note: this wont effect resources that are only parsed(strings and game parameters)
		void refresh_impl() override;
		//adds a specific resource into the load queue
		void refresh_impl(unique_id) override;

		void abandon_refresh_impl(unique_id, std::optional<unique_id>) override;

		//adds a range to the load queue
		//template<Iter>
		//void refresh(Iter first, Iter last);

		// TODO: move the loadqueue to data_manager
		//loads all queued resources
		void load();
		//loads a number of objects from the queue(no order specified)
		void load(types::uint8 count);
		//loads from the queue for a specified time
		//returns the number of objects loaded
		//>>types::uint32 load(Time ms);

		//loads a specific object
		void load(unique_id);

		//loads a range of objects(only if they are on the queue)
		//template<Iter>
		//void load(Iter first, Iter last);

		void export_mod_impl(unique_id, std::string_view) override;

		//convert string to uid
		const types::string& get_as_string_impl(unique_id id) const noexcept override;
		unique_id get_uid_impl(std::string_view name) const override;
		unique_id get_uid_impl(std::string_view name) override;

	protected:
		const std::filesystem::path& _current_data_file() const noexcept override
		{
			return _data_file;
		}

	private:
		template<typename Stream>
		void _write_mod(const mod&, const std::filesystem::path&, Stream&);
		void _parse_mod(unique_id id, std::string_view source, const data::parser_node& modRoot, bool load_deps);
		void parseYaml(unique_id, const data::parser_node&);
		
		//==parsing and loading data==
		unordered_map_string<resources::parser_func> _resourceParsers;
		std::filesystem::path _data_file;
		//==stored resource data==
		//list of used names
		unique_id _game = unique_zero;
		std::vector<unique_id> _mods;
		[[deprecated]] unique_id _leaf_source = unique_zero; // temp loaded mod, this is the id of a mission file
		//map of names to Uids
		unordered_map_string<unique_id> _ids;
		//list of unloaded resources
		std::mutex _load_mutex;
		std::vector<resources::resource_base*> _loadQueue;
	};
}

#include "hades/detail/data_system.inl"

#endif // hades_data_hpp
