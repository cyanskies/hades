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

//systems are used to modify the game state or the generate a frame for drawing

//when a system is created on_create is called, and the currently attached ents are passed to it
// on_create should be used to set up any background state(quad_maps or object registeries) that isn't stored in the game state

//when an object is attached to a system, on_connect is called
// on_connect is used to update background state, and invoke one off changes to the game state for objects
// on_connect is only ever called once for each object

//on_tick is called every game tick for any object attached to a system
// on_tick is for general game logic
// on_connect is always called for an object before on_tick
// TODO: on_tick is always called for an object at least once? even if the object is destroyed by its on_connect handler?
// TODO: can on_connect and on_tick occur on the same tick?

//on_diconnect is called when an object is detached
// it might spawn a death animation
// disconenct the object from background states
// no other on_* function will be called for this object after on_disconnect

//on_destroy can be used to clean up background state
// typically unused

// fwds
namespace hades
{
	struct terrain_map;
}

namespace hades
{
	void register_game_system_resources(data::data_manager&);

	class system_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	using system_data_t = std::any;
	using attached_ent = std::pair<entity_id, time_point>;
	using name_list = curve<std::vector<attached_ent>>;

	template<typename SystemType>
	class system_behaviours
	{
	public:
		using system_type = SystemType;
		using system_resource = typename SystemType::system_t;

		std::vector<SystemType>& get_systems() noexcept
		{
			return _systems;
		}

		std::vector<const system_resource*> get_new_systems() noexcept
		{
			auto ret = std::vector<const system_resource*>{};
			std::swap(ret, _new_systems);
			return ret;
		}

		system_data_t& get_system_data(unique_id key)
		{
			return _system_data[key];
		}

		void set_attached(unique_id, name_list);

		// call if time scrubbing, will set the new and removed ents
		// correctly
		void set_current_time(time_point);

		//get entites that have been added to the system
		//since the last frame
		std::vector<entity_id> get_new_entities(SystemType&);
		//get all entities currently attached to the system
		const name_list& get_entities(SystemType&) const;
		//get entities that were removed from the system last frame
		std::vector<entity_id> get_removed_entities(SystemType&);

		void attach_system(entity_id, unique_id, time_point t);
		void detach_system(entity_id, unique_id, time_point t);
		//remove this entity from all systems
		void detach_all(entity_id, time_point t);

		//this entity won't trigger on_tick events untill the provided time point
		void sleep_entity(entity_id, unique_id, time_point from, time_point until);

	private:
		std::vector<SystemType> _systems;
		std::vector<const system_resource*> _new_systems;
		std::unordered_map<unique_id, system_data_t> _system_data;
	};

	//fwd
	class game_interface;
	struct game_system;

	struct system_job_data
	{
		unique_id system = unique_id::zero;
		//entity to run on
		std::vector<entity_id> entity;
		//level data interface:
		// contains units, particles, buildings, terrain
		// per level quests and objectives
		game_interface *level_data = nullptr;
		//TODO: map of uniqiue id to other game interface ads
		//mission data interface
		// contains players, 
		// and... just the players
		game_interface *mission_data = nullptr;

		system_behaviours<game_system>* systems = nullptr;

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

			system_func on_create, //called on system creation
				on_connect,			//called when attached to an entity
				on_disconnect,     //called when detatched from ent
				tick,				//called every tick
				on_destroy;			//called on system destruction
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

		std::vector<entity_id> new_ents;
		std::vector<entity_id> removed_ents;
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
		std::vector<entity_id> entity;
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
		// dt?
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
		name_list attached_entities{ curve_type::step };
		std::vector<entity_id> new_ents;
		std::vector<entity_id> removed_ents;
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
	// game units are world pixels expressed in floats
	using world_unit_t = resources::curve_types::float_t;
	using world_vector_t = resources::curve_types::vec2_float;
	using world_rect_t = rect_t<world_unit_t>;

	//TODO: move to game_system_interface
	//functions for game state access

	//funcs to call before a system gets control, and to clean up after
	void set_game_data(system_job_data*) noexcept;

	//game contains general functions available to game systems
	namespace game
	{
		using namespace resources::curve_types;

		template<typename T>
		using curve_keyframe = keyframe<T>;

		const std::vector<object_ref> &get_objects() noexcept;

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
		//world data

		//object data

		//common curves

		//curves access

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

		//==level local values==
		// these are not sent to clients or saved
		// used for level local system data that is needed
		// by multiple systems
		// eg. collision trees
		template<typename T>
		T &get_level_local_ref(unique_id);

		//==world data==
		world_rect_t get_world_bounds();
		//get terrain data
		//get tile info?(tags)

		//==object data==
		//NOTE: for the following functions(get/set curves/values)
		// if the object_ref is not provided, it is set to game::get_object()
		// if the time_point is not provided, it is set to game::get_last_time() for get_* functions
		// and game::get_time() for set_* functions
		
		object_ref get_object_from_name(std::string_view, time_point);

		template<typename T>
		const curve<T> &get_curve(curve_index_t);

		//gets the keyframe on or befre the time point
		//use set_value to write a keyframe
		template<typename T>
		const curve_keyframe<T>& get_keyframe_ref(curve_index_t, time_point);
		
		template<typename T>
		curve_keyframe<T> get_keyframe(curve_index_t, time_point);
		template<typename T>
		curve_keyframe<T> get_keyframe(curve_index_t);
		
		//returns a reference to the value stored in a curve at a specific time
		// it is invalid to get a reference to a linear curve
		template<typename T>
		const T& get_ref(curve_index_t, time_point);
		template<typename T>
		const T& get_ref(curve_index_t);
		
		template<typename T>
		T get_value(curve_index_t, time_point);
		template<typename T>
		T get_value(curve_index_t);
		
		template<typename T>
		void set_curve(curve_index_t, curve<T>);
		
		//seting value will remove all keyframes after time_point
		template<typename T>
		void set_value(curve_index_t, time_point, T&&);
		
		//object creation and destruction
		//an object is always created at game::get_time() time.
		//TODO: resolve these immidiatly, both creation/destruction and //attach/detach
		object_ref create_object(object_instance);

		//system config
		void attach_system(object_ref, unique_id, time_point);
		void sleep_system(object_ref, unique_id, time_point from, time_point until);
		void detach_system(object_ref, unique_id, time_point);

		//removes all systems from object
		void destroy_object(object_ref, time_point);

		//helper functions for accessing common curves
		//position
		world_vector_t get_position(object_ref, time_point);
		world_vector_t get_position(object_ref);
		void set_position(object_ref, world_unit_t x, world_unit_t y, time_point);
		
		//size
		world_vector_t get_size(object_ref, time_point);
		world_vector_t get_size(object_ref);
		void set_size(object_ref, world_unit_t w, world_unit_t h, time_point);
	}

	void set_render_data(render_job_data*) noexcept;

	//render access functions allow a const view of the game state
	namespace render
	{
		using namespace resources::curve_types;
		const std::vector<object_ref> &get_objects();
		render_interface *get_render_output();
		time_point get_time();

		template<typename T>
		T &get_system_data();
		template<typename T>
		void set_system_data(T value);
		void destroy_system_data();

		//TODO: drawing functions

		//create/destroy sprite
		//set_sprite
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
		const curve<T>& get_curve(curve_index_t);
		template<typename T>
		const T& get_ref(curve_index_t, time_point);
		template<typename T>
		const T get_value(curve_index_t, time_point);

		world_rect_t get_world_bounds();
		const terrain_map& get_world_terrain() noexcept;
		//get level data

		//common curves
		world_vector_t get_position(object_ref, time_point);
		world_vector_t get_position(object_ref);

		world_vector_t get_size(object_ref, time_point);
		world_vector_t get_size(object_ref);
	}
}

#include "hades/detail/game_system.inl"

#endif //!HADES_GAMESYSTEM_HPP
