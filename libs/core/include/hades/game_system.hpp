#ifndef HADES_GAME_SYSTEM_HPP
#define HADES_GAME_SYSTEM_HPP

#include <any>
#include <functional>
#include <vector>

#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/exceptions.hpp"
#include "hades/game_types.hpp"
#include "hades/level_curve_data.hpp"
#include "hades/objects.hpp"
#include "hades/resource_base.hpp"
#include "hades/time.hpp"

namespace hades
{
	void register_game_system_resources(data::data_manager&);

	class system_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	using system_data_t = std::any;

	template<typename SystemType>
	class system_behaviours
	{
	public:
		using system_type = SystemType;
		using system_resource = typename SystemType::system_t;

		std::vector<SystemType> get_systems() const
		{
			return _systems;
		}

		std::vector<const system_resource*> get_new_systems() const
		{
			return _new_systems;
		}

		void clear_new_systems() noexcept
		{
			_new_systems.clear();
		}

		system_data_t& get_system_data(unique_id key)
		{
			return _system_data[key];
		}

		//TODO: should systems be considered static for entity lifetime?
		void attach_system(entity_id, unique_id, time_point t);
		void detach_system(entity_id, unique_id, time_point t);

		//TODO: sleep_ent(count || duration)
		//TODO: wake_ents(dt)

	private:
		std::vector<SystemType> _systems;
		std::vector<const system_resource*> _new_systems;
		std::unordered_map<unique_id, system_data_t> _system_data;
	};

	//fwd
	class game_interface;

	struct system_job_data
	{
		//entity to run on
		entity_id entity = bad_entity;
		//level data interface:
		// contains units, particles, buildings, terrain
		// per level quests and objectives
		game_interface *level_data = nullptr;
		//TODO: map of uniqiue id to other game interface ads
		//mission data interface
		// contains players, 
		// and... just the players
		game_interface *mission_data = nullptr;
		//the previous time, and the time to advance by(t + dt)
		time_point prev_time;
		time_duration dt;
		// current_time = prev_time + dt

		//system data
		system_data_t *system_data = nullptr;
	};

	namespace resources
	{
		struct system_t
		{};

		//a system stores a job function
		struct system : public resource_type<system_t>
		{
			//TODO: dont accept job_data anymoe, same for render system
			using system_func = std::function<void()>;

			system_func on_create, //called on system creation(or large time leap)
				on_connect,			//called when attached to an entity(or large time leap)
				on_disconnect,     //called when detatched from ent
				tick,				//called every tick
				on_destroy;			//called on system destruction(or large time leap, before on_* functions)
			//	on_event?

			//std::any system_info; //stores the system object or script reference.
			//if loaded from a manifest then it should be loaded from scripts
			//if it's provided by the application, then source is empty, and no laoder function is provided.
		};
	}

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	void make_system(unique_id id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager&);

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	void make_system(std::string_view name, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &data)
	{
		return make_system(data.get_uid(name), on_create, on_connect, on_disconnect, on_tick, on_destroy, data);
	}

	using name_list = curve<resources::curve_types::collection_object_ref>;
	
	resources::curve_types::collection_object_ref get_added_entites(const name_list&, time_point last_frame, time_point this_frame);
	resources::curve_types::collection_object_ref get_removed_entites(const name_list&, time_point last_frame, time_point this_frame);
	
	//the interface for game systems.
	//systems work by creating jobs and passing along the data they will use.
	struct game_system
	{
		using system_t = resources::system;
		using job_data_t = system_job_data;

		explicit game_system(const resources::system* s) : system(s)
		{}

		game_system(const resources::system* s, name_list nl) : system{ s }, attached_entities{ std::move(nl) }
		{}

		game_system(const game_system&) = default;
		game_system(game_system&&) noexcept = default;

		game_system &operator=(const game_system&) = default;
		game_system &operator=(game_system&&) = default;

		//this holds the systems, name and id, and the function that the system uses.
		const resources::system* system = nullptr;
		//list of entities attached to this system, over time
		name_list attached_entities{ curve_type::step };
	};

	//program provided systems should be attatched to the renderer or 
	//gameinstance depending on what kind of system they are

	//scripted systems should be listed in the game_system: and render_system: lists in
	//the mod files that added them

	class render_interface;
	struct render_system;
	class common_interface;

	struct render_job_data
	{
		//the system currently running
		unique_id system = unique_id::zero;
		//entity to run on
		entity_id entity = bad_entity;
		//level data interface:
		// contains units, particles, buildings, terrain
		// per level quests and objectives
		const common_interface *level_data = nullptr;
		//mission data interface
		// contains players, 
		// and... just the players
		//game_interface *mission_data = nullptr;

		//for sleeping system updates
		system_behaviours<render_system>* systems = nullptr;

		//the current time
		time_point current_time;
		//render output interface
		render_interface *render_output = nullptr;
		//system data
		system_data_t *system_data = nullptr;
	};

	namespace resources
	{
		struct render_system_t
		{};

		struct render_system : public resource_type<render_system_t>
		{
			using system_func = std::function<void()>;

			system_func on_create,
				on_connect,
				on_disconnect,
				tick,
				on_destroy;

			//std::any system_info;
		};
	}

	struct render_system
	{
		using system_t = resources::render_system;
		using job_data_t = render_job_data;

		explicit render_system(const resources::render_system* s) : system(s)
		{}

		render_system(const resources::render_system* s, name_list nl) : system{ s }, attached_entities{ std::move(nl) }
		{}

		render_system(const render_system&) = default;
		render_system(render_system&&) noexcept = default;

		render_system& operator=(const render_system&) = default;
		render_system& operator=(render_system&&) = default;

		//this holds the systems, name and id, and the function that the system uses.
		const resources::render_system *system = nullptr;
		//list of entities attached to this system, over time
		name_list attached_entities{ curve_type::step };
	};

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	void make_render_system(unique_id id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager&);
	
	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	void make_render_system(std::string_view id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &d)
	{
		make_render_system(d.get_uid(id), on_create, on_connect, on_disconnect, on_tick, on_destroy, d);
	}

	
	//extra types that cannot be defined in
	//game_types.hpp due to circular includes

	//world unit type
	//used for position ranges and sizes
	//game_unit is used to measure distance in the game world
	using world_unit_t = resources::curve_types::float_t;
	using world_vector_t = vector_t<world_unit_t>;
	using world_rect_t = rect_t<world_unit_t>;

	//TODO: move to level_interface?
	//functions for game state access

	//funcs to call before a system gets control, and to clean up after
	void set_game_data(system_job_data*) noexcept;

	//game contains general functions available to game systems
	namespace game
	{
		using namespace resources::curve_types;

		template<typename T>
		struct curve_keyframe
		{
			T value;
			time_point time;
		};

		object_ref get_object() noexcept;

		//the time of the previous update
		time_point get_last_time() noexcept;
		//the time between this update and the previous one
		time_duration get_delta_time() noexcept;
		//the current time
		//we need to calculate the state at this time
		//using values from last_time
		time_point get_time() noexcept;

		//system data is a data store persisted between frames
		// it is per-level and is never saved
		template<typename T>
		T &get_system_data();
		template<typename T>
		void set_system_data(T&& value);
		void destroy_system_data();
	}

	//game::mission contains functions for accessing mission state
	namespace game::mission
	{
		//get_curve_x
		//set_curve_x

		//NOTE: get/set_value simplyfy access to linear, const and step curves
		//		pulse curves, or any work with keyframes requires accessing the actual curve
 		//get_value
		//set_value
		//get_level_index(level_id)
	}

	//game::level for accessing and mutating the current level state
	//	the current level is the one containing game::get_object()
	//	you can change to a different level with the change_level function

	namespace game::level
	{
		//void return_level() //restores the origional level
		//void switch_level(level_id)

		//NOTE: for the following functions(get/set curves/values)
		// if the object_ref is not provided, it is set to game::get_object()
		// if the time_point is not provided, it is set to game::get_last_time() for get_* functions
		// and game::get_time() for set_* functions
		
		object_ref get_object_from_name(std::string_view, time_point);

		template<typename T>
		curve<T> get_curve(object_ref, variable_id);
		template<typename T>
		curve<T> get_curve(curve_index_t);
		template<typename T>
		curve<T> get_curve(variable_id);

		//gets the keyframe on or befre the time point
		//use set_value to write a keyframe
		template<typename T>
		curve_keyframe<T> get_keyframe(curve_index_t, time_point);
		template<typename T>
		curve_keyframe<T> get_keyframe(object_ref, variable_id, time_point);
		template<typename T>
		curve_keyframe<T> get_keyframe(variable_id, time_point);
		template<typename T>
		curve_keyframe<T> get_keyframe(curve_index_t);
		template<typename T>
		curve_keyframe<T> get_keyframe(object_ref, variable_id);
		template<typename T>
		curve_keyframe<T> get_keyframe(variable_id);

		template<typename T>
		T get_value(object_ref, variable_id, time_point);
		template<typename T>
		T get_value(curve_index_t, time_point);
		template<typename T>
		T get_value(variable_id, time_point);
		template<typename T>
		T get_value(object_ref, variable_id);
		template<typename T>
		T get_value(curve_index_t);
		template<typename T>
		T get_value(variable_id);

		template<typename T>
		void set_curve(object_ref, variable_id, curve<T>);
		template<typename T>
		void set_curve(curve_index_t, curve<T>);
		template<typename T>
		void set_curve(variable_id, curve<T>);

		//seting value will remove all keyframes after time_point
		template<typename T>
		void set_value(object_ref, variable_id, time_point, T&&);
		template<typename T>
		void set_value(curve_index_t, time_point, T&&);
		template<typename T>
		void set_value(variable_id, time_point, T&&);
		template<typename T>
		void set_value(object_ref, variable_id, T&&);
		template<typename T>
		void set_value(curve_index_t, T&&);
		template<typename T>
		void set_value(variable_id v, T&&);

		//an object is always created at game::get_time() time.
		object_ref create_object(object_instance);

		//helper functions for accessing common curves
		world_rect_t get_world_bounds();

		//position
		world_vector_t get_position(object_ref, time_point);
		world_vector_t get_position(object_ref);
		world_vector_t get_position();
		void set_position(object_ref, world_unit_t x, world_unit_t y, time_point);
		void set_position(object_ref, world_unit_t x, world_unit_t y);
		void set_position(world_unit_t x, world_unit_t y);

		//size
		world_vector_t get_size(object_ref, time_point);
		world_vector_t get_size(object_ref);
		world_vector_t get_size();
		void set_size(object_ref, world_unit_t w, world_unit_t h, time_point);
		void set_size(object_ref, world_unit_t w, world_unit_t h);
		void set_size(world_unit_t w, world_unit_t h);
	}

	void set_render_data(render_job_data*) noexcept;

	//render access functions allow a const view of the game state
	namespace render
	{
		using namespace resources::curve_types;
		object_ref get_object();
		render_interface *get_render_output();
		time_point get_time();

		template<typename T>
		T &get_system_data();
		template<typename T>
		void set_system_data(T value);
		void destroy_system_data();

		//drawing functions
	}

	namespace render::mission
	{
		/*template<typename T>
		T get_curve(entity_id, variable_id);
		
		template<typename T>
		void set_curve(entity_id, variable_id, T&& value);*/
	}

	namespace render::level
	{
		//get curve from this level
		template<typename T>
		const curve<T>& get_curve(variable_id);

		template<typename T>
		const curve<T>& get_curve(curve_index_t);

		template<typename T>
		inline const curve<T> &get_curve(object_ref e, variable_id v)
		{ return get_curve<T>(curve_index_t{ e, v }); }

		template<typename T>
		const T &get_value(curve_index_t, time_point);

		//render systems are currently read only
		/*template<typename T>
		void set_curve(variable_id, T&& value);

		template<typename T>
		void set_curve(entity_id, variable_id, T&& value);*/

		//get curve from a different level
		/*template<typename T>
		T get_curve(unique_id, curve_index_t);
		
		template<typename T>
		T get_curve(unique_id, entity_id, variable_id);*/

		/*template<typename T>
		void set_curve(curve_index_t, T&& value);

		template<typename T>
		void set_curve(unique_id, curve_index_t, T&& value);*/
	}
}

#include "hades/detail/game_system.inl"

#endif //!HADES_GAMESYSTEM_HPP