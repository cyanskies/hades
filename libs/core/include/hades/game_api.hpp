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
	//game contains general functions available to game systems
	namespace game
	{
		using namespace curve_types;

		template<typename T>
		using curve_keyframe = keyframe<T>;
		template<typename T>
		using optional_keyframe = std::optional<curve_keyframe<T>>;

		const std::vector<object_ref> &get_objects() noexcept;

		//the time of the previous update
		time_point get_last_time() noexcept;
		//the time between this 'last_time' and 'time'
		time_duration get_delta_time() noexcept;
		//the current time
		//we need to calculate the state at this time
		//using values from last_time
		time_point get_time() noexcept;

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
		//world bounds in pixels
		world_rect_t get_world_bounds();
		const terrain_map& get_world_terrain() noexcept;
		//get tile info?(tags)

		//==object data==
		//NOTE: for the following functions(get/set curves/values)
		// if the object_ref is not provided, it is set to game::get_object()
		// if the time_point is not provided, it is set to game::get_last_time() for get_* functions
		// and game::get_time() for set_* functions
		
		object_ref get_object_from_name(std::string_view, time_point);

		//=== Curves ===
		//get access to curve object
		template<typename T>
		const curve<T> &get_curve(curve_index_t);

		//=== Keyframes ===
		//gets the keyframe on or befre the time point
		//use set_value to write a keyframe
		template<typename T>
		const curve_keyframe<T> *get_keyframe_ref(curve_index_t, time_point); // returns non-owning ptr
		template<typename T>
		optional_keyframe<T> get_keyframe(curve_index_t, time_point);
		template<typename T>
		optional_keyframe<T> get_keyframe(curve_index_t);
		//get the keyframe after the time point
		template<typename T>
		const curve_keyframe<T> *get_keyframe_after_ref(curve_index_t, time_point); // returns non-owning ptr
		template<typename T>
		optional_keyframe<T> get_keyframe_after(curve_index_t, time_point);
		template<typename T>
		optional_keyframe<T> get_keyframe_after(curve_index_t);
		//get keyframes either side of the time_point
		// if their is a timeframe exactly at that time_point
		// then returns that frame and the one after
		template<typename T>
		std::pair<const curve_keyframe<T>*, const curve_keyframe<T>*> get_keyframe_pair_ref(curve_index_t, time_point);
		template<typename T>
		std::pair<optional_keyframe<T>, optional_keyframe<T>> get_keyframe_pair(curve_index_t, time_point);
		template<typename T>
		std::pair<optional_keyframe<T>, optional_keyframe<T>> get_keyframe_pair(curve_index_t);

		//=== Refs ===
		//returns a reference to the value stored in a curve at a specific time
		// it is invalid to get a reference to a linear curve
		template<typename T>
		const T& get_ref(curve_index_t, time_point);
		template<typename T>
		const T& get_ref(curve_index_t);
		
		//=== Values ===
		template<typename T>
		T get_value(curve_index_t, time_point);
		template<typename T>
		T get_value(curve_index_t);
		
		//=== Set ==
		template<typename T>
		void set_curve(curve_index_t, curve<T>);
		//seting value will remove all keyframes after time_point
		template<typename T>
		void set_value(curve_index_t, time_point, T&&);
		
		//=== Object Controls ===
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

		//=== Common Curves ===
		//position
		world_vector_t get_position(object_ref, time_point);
		world_vector_t get_position(object_ref);
		void set_position(object_ref, world_vector_t, time_point);
		void set_position(object_ref, world_vector_t);
		//size
		world_vector_t get_size(object_ref, time_point);
		world_vector_t get_size(object_ref);
		[[deprecated]]
		void set_size(object_ref, world_unit_t w, world_unit_t h, time_point); // TODO: remove me
		void set_size(object_ref, world_vector_t, time_point);
		void set_size(object_ref, world_vector_t);
		//tags
		bool tags(object_ref, tag_list has, tag_list not);
		//void tags_add(object_ref, tag_list);
		//void tags_remove(object_ref, tag_list);

	}

	//render access functions allow a const view of the game state
	namespace render //TODO: client = render?
	{
		using namespace curve_types;
		const std::vector<object_ref> &get_objects() noexcept;
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

		void destroy(id_t);

		bool exists(id_t) noexcept;

		//replace all the sprite data for id
		void set(id_t, const resources::animation*, time_point,
			layer_t, vector_float position, vector_float size);
		//update the commonly changing data
		void set(id_t, time_point, vector_float position, vector_float size);
		//update specific data
		/*void set_animation(id_t, const resources::animation*, time_point);
		void set_layer(id_t, layer_t position);
		void set_size(id_t, vector_float size);*/
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

		//get curve from this level
		template<typename T>
		const curve<T>& get_curve(curve_index_t);
		template<typename T>
		const T& get_ref(curve_index_t, time_point);
		template<typename T>
		const T& get_ref(curve_index_t);
		template<typename T>
		const T get_value(curve_index_t, time_point);
		template<typename T>
		const T get_value(curve_index_t);

		//world info
		world_rect_t get_world_bounds();
		const terrain_map& get_world_terrain() noexcept;

		//common curves
		world_vector_t get_position(object_ref, time_point);
		world_vector_t get_position(object_ref);

		world_vector_t get_size(object_ref, time_point);
		world_vector_t get_size(object_ref);

		//returns true if the object has the tags from the 'has' list
		// and doesn't have the tags from 'not'
		bool tags(object_ref, const tag_list& has, const tag_list& not);

		//tag info
		inline bool has_tag(object_ref o, unique u)
		{
			return has_tag(o, { u });
		}

		inline bool has_tag(object_ref o, const tag_list& u)
		{
			return tags(o, u, {});
		}

		inline bool not_tag(object_ref o, unique u)
		{
			return not_tag(o, { u });
		}

		inline bool not_tag(object_ref o, const tag_list& u)
		{
			return tags(o, {}, u);
		}
	}
}

#include "hades/detail/game_api.inl"

#endif //!HADES_GAME_API_HPP
