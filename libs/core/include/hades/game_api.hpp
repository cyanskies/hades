#ifndef HADES_GAME_API_HPP
#define HADES_GAME_API_HPP

#include <optional>

#include "hades/curve_extra.hpp"
#include "hades/curve_types.hpp"
#include "hades/game_types.hpp"
#include "hades/render_interface.hpp"
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
namespace sf
{
	class Drawable; //for hades::render::drawable::create_ptr/update_ptr
}

namespace hades
{
	struct terrain_map;
}

namespace hades
{
	struct curve_index_t 
	{
		object_ref o;
		variable_id curve;
	};

	//game contains general functions available to game systems
	namespace game
	{
		using namespace curve_types;
		std::vector<object_ref> &get_objects() noexcept;

		//the time of the previous dt
		// equals get_time - dt
		time_point get_last_time() noexcept;
		//the time between this 'last_time' and 'time'
		time_duration get_delta_time() noexcept;
		//the current time
		//we need to calculate the state at this time
		//using values from last_time
		time_point get_time() noexcept;

		const std::vector<player_data>& get_players() noexcept;
		object_ref get_player(unique) noexcept;

		unique_id current_system() noexcept;
		//system data is a data store persisted between frames
		// it is per-level and is never saved
		template<typename T>
		T &get_system_data();
		template<typename T>
		void set_system_data(T value);
		void destroy_system_data();
	}

	//game::mission contains functions for accessing mission state
	namespace game::mission
	{
		//world data

		//object data

		//common curves

		//curves access
		//template<typename T>
		//const game_property_curve<T>& get_curve(curve_index_t);

		//curve values
		template<typename T>
		T get_value(curve_index_t, time_point);
		template<typename T>
		T get_value(curve_index_t);

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
		T& get_level_local_ref(unique_id);
		template<typename T>
		void set_level_local_value(unique_id, T value);

		//==world data==
		//world bounds in pixels
		world_rect_t get_world_bounds();
		const terrain_map& get_world_terrain() noexcept;
		//get tile info?(tags)

		//==object data==
		//NOTE: for the following functions(get/set curves/values)
		// if the time_point is not provided, it is set to game::get_last_time() for get_* functions
		// and game::get_time() for set_* functions
		
		namespace object
		{
			//find from name
			object_ref get_from_name(std::string_view, time_point);

			//property access
			//TODO: rename to curve_ref
			template<template<typename> typename CurveType, typename T>
			CurveType<T>& get_property_ref(object_ref&, variable_id);
			template<template<typename> typename CurveType, typename T>
			CurveType<T>& get_property_ref(const object_ref& o, variable_id v)
			{
				auto obj = o;
				return get_property_ref<CurveType, T>(obj, v);
			}

			//creation and destruction
			object_ref create(const object_instance&);
			object_ref clone(object_ref);
			void destroy(object_ref);

			//pauses this system update calls for this object until the provided time
			void sleep_system(object_ref, time_point);

			time_point get_creation_time(object_ref);
			const tag_list& get_tags(object_ref);

			//NOTE: this isn't a curve anymore
			bool is_alive(object_ref) noexcept;

			// common curves
			linear_curve<vec2_float>& get_position(object_ref);
			vec2_float get_size(object_ref);
		}

		template<std::size_t Size>
		std::array<bool, Size> check_tags(object_ref o, std::array<unique_id, Size> t)
		{
			const auto& tags = hades::game::level::object::get_tags(o);
			return hades::check_tags(tags, t);
		}

		inline bool check_tag(object_ref o, unique_id t)
		{
			return check_tags(o, std::array{ t })[0];
		}
	}

	//render access functions allow a const view of the game state
	namespace render //TODO: client = render?
	{
		using namespace curve_types;
		std::vector<object_ref> &get_objects() noexcept;
		render_interface *get_render_output() noexcept;
		time_point get_time() noexcept;

		template<typename T>
		T &get_system_data() noexcept;
		template<typename T>
		void set_system_data(T value);
		void destroy_system_data() noexcept;
	}

	namespace render::sprite
	{
		// NOTE: any of the following functions that accept a id_t param
		// may throw render_interface_invalid_id if the id doesn't
		// resolve to a known sprite.
		using id_t = render_interface::sprite_id;
		using layer_t = render_interface::sprite_layer;

		id_t create();
		id_t create(const resources::animation*, time_point,
			layer_t, vector_float position, vector_float size);
		id_t create(const resources::animation*, float progress, layer_t,
			vector_float position, vector_float size);

		void destroy(id_t);

		bool exists(id_t) noexcept;

		//replace all the sprite data for id
		void set(id_t, const resources::animation*, time_point,
			layer_t, vector_float position, vector_float size);
		void set(id_t, const resources::animation*, float progress,
			layer_t, vector_float position, vector_float size);
		//update the commonly changing data
		void set(id_t, time_point, vector_float position, vector_float size);
		void set(id_t, float progress, vector_float position, vector_float size);
		void set_animation(id_t, const resources::animation*, time_point);
		void set_animation(id_t, const resources::animation*, float progress);
		void set_animation(id_t, time_point);
		void set_animation(id_t, float progress);
	}

	namespace render::drawable
	{
		// NOTE: any of the following functions that accept a id_t param
		// may throw render_interface_invalid_id if the id doesn't
		// resolve to a known sprite.
		using id_t = render_interface::drawable_id;
		using layer_t = render_interface::drawable_layer;

		id_t create();

		// NOTE: the *_ptr functions do not take ownership of the object
		// you must make sure the object is stored at a stable address
		// and outlives the drawable registered here.
		id_t create_ptr(const sf::Drawable*, layer_t);
		template<typename DrawableObject>
		id_t create(DrawableObject&&, layer_t);

		bool exists(id_t) noexcept;

		void update_ptr(id_t, const sf::Drawable*, layer_t);
		template<typename DrawableObject>
		void update(id_t, DrawableObject&&, layer_t);

		void destroy(id_t);
	}

	namespace render::audio
	{
		//samples(short audios)

		// plays a sample once globally
		void play(/*sample id*/);
		// as above
		// sample ends early if the entity is destroyed
		void play(/*entity_id*/ /*sample id*/ /*loop*/);
		void stop(/*entity_id*/ /*sample id*/);

		//jukebox(long audio)
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
		//==level local values==
		// these are not synced to servers or saved
		// used for level local system data that is needed
		// by multiple systems (usually for performance)
		// eg. collision trees
		template<typename T>
		T& get_level_local_ref(unique_id);
		template<typename T>
		void set_level_local_value(unique_id, T value); // NOTE: should we return T& from this?

		//world info
		world_rect_t get_world_bounds();
		const terrain_map& get_world_terrain() noexcept;

		const resources::object* get_object_type(object_ref) noexcept;
		namespace object
		{
			time_point get_creation_time(object_ref);
			template<template<typename> typename CurveType, typename T>
			const CurveType<T>& get_property_ref(object_ref, variable_id);

			const hades::linear_curve<vec2_float>& get_position(object_ref);
			vec2_float get_size(object_ref);
		}
	}
}

#include "hades/detail/game_api.inl"

#endif //!HADES_GAME_API_HPP
