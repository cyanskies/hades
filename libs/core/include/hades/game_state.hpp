#ifndef HADES_GAME_STATE_HPP
#define HADES_GAME_STATE_HPP

#include "plf_colony.h"

#include "hades/any_map.hpp"
#include "hades/curve_types.hpp"
#include "hades/game_system.hpp"
#include "hades/uniqueid.hpp"

namespace hades
{
	//this isn't needed for EntityId's and Entity names are strings, and rarely used, where
	//curves need to be identified often by a consistant lookup name
	//we do the same with variable Ids since they also need to be unique and easily network transferrable
	using variable_id = unique_id;
	constexpr auto bad_variable = variable_id{ variable_id::zero_id };

	template<typename DataType>
	struct state_field
	{
		entity_id object = bad_entity;
		variable_id id = bad_variable;
		DataType data;
	};

	namespace detail
	{
		// a fancy ref to the variable entry
		template<typename T>
		struct var_entry
		{
			using value_type = T;
			variable_id id = bad_variable;
			state_field<T>* var = nullptr;
		};

		template<typename T>
		constexpr bool operator==(const var_entry<T>& l, const var_entry<T>& r) noexcept
		{
			return l.id == r.ld;
		}		

		template<typename T>
		struct entity_var_type;

		template<typename... Ts>
		struct entity_var_type<std::tuple<Ts...>>
		{
			//expands a type out for a vector of that type in each style of curve
			template<typename T>
			using tuple_curve_vector_t = std::conditional_t<curve_types::is_linear_interpable_v<T>,
				std::tuple<
				std::vector<var_entry<linear_curve<T>>>,
				std::vector<var_entry<step_curve<T>>>,
				std::vector<var_entry<pulse_curve<T>>>,
				std::vector<var_entry<const_curve<T>>>
				>,
				std::tuple <
				std::vector<var_entry<step_curve<T>>>,
				std::vector<var_entry<pulse_curve<T>>>,
				std::vector<var_entry<const_curve<T>>>
				>
			>;

			using type = decltype(std::tuple_cat(std::declval<tuple_curve_vector_t<Ts>>()...));
		};

		template<typename T>
		using entity_var_type_t = typename entity_var_type<T>::type;
	}

	namespace resources
	{
		struct object;
	}

	//authoratative game object
	struct game_obj
	{
		entity_id id = bad_entity;
		const resources::object* object_type = nullptr; //tells what variables a entity has, and which systems control it.

		template<template<typename> typename CurveType, typename T>
		using var_entry = detail::var_entry<CurveType<T>>;
		template<template<typename> typename CurveType, typename T>
		using var_list = std::vector<var_entry<CurveType, T>>;

		using entity_variable_lists_t = detail::entity_var_type_t<curve_types::type_pack>;
		entity_variable_lists_t object_variables;
	};

	inline constexpr bool operator==(const game_obj& lhs, const game_obj &rhs) noexcept
	{
		return lhs.id == rhs.id;
	}

	using object_name_map = std::unordered_map<string, step_curve<object_ref>>;

	namespace detail
	{		
		template<typename T> struct state_data_type;
		template<typename... Ts>
		struct state_data_type<std::tuple<Ts...>>
		{
			template<typename T>
			using tuple_curve_colony_t = std::conditional_t<curve_types::is_linear_interpable_v<T>,
				std::tuple<
				plf::colony<state_field<linear_curve<T>>>,
				plf::colony<state_field<step_curve<T>>>,
				plf::colony<state_field<pulse_curve<T>>>,
				plf::colony<state_field<const_curve<T>>>
				>,
				std::tuple <
				plf::colony<state_field<step_curve<T>>>,
				plf::colony<state_field<pulse_curve<T>>>,
				plf::colony<state_field<const_curve<T>>>
				>
			>;

			using type = decltype(std::tuple_cat(std::declval<tuple_curve_colony_t<Ts>>()...));
		};

		template<typename T>
		using state_data_type_t = typename state_data_type<T>::type;

		//we don't shrink the memory space
		// we need old ptrs to remain valid, but be detectable as stale
		// after being replaced
		class game_object_collection
		{
		public:
			static constexpr auto empty_flag = false;
			static constexpr auto not_empty_flag = true;

			class iterator
			{
			public:
				using v_bool_iter = std::vector<bool>::iterator;
				using v_obj_iter = std::deque<game_obj>::iterator;
				using iterator_category = v_obj_iter::iterator_category;
				using value_type = game_obj;
				using difference_type = v_obj_iter::difference_type;
				using pointer = v_obj_iter::pointer;
				using reference = v_obj_iter::reference;

				iterator(v_bool_iter a, v_bool_iter a_end, v_obj_iter b) noexcept :
					_empty_iter{ a }, _empty_end{ a_end }, _obj_iter{ b }
				{
					while (_empty_iter != _empty_end  && *_empty_iter == empty_flag)
					{
						++_empty_iter; ++_obj_iter;
					}
				}

				bool operator!=(const iterator& rhs) const noexcept
				{
					return _empty_iter != rhs._empty_iter;
				}

				game_obj& operator*() const
				{
					return *_obj_iter;
				}

				game_obj* operator->() const
				{
					return &*_obj_iter;
				}

				iterator& operator++()
				{
					++_empty_iter;
					++_obj_iter;
					while (_empty_iter != _empty_end && *_empty_iter == empty_flag)
					{
						++_empty_iter; ++_obj_iter;
					}
					return *this;
				}

				iterator operator++(int)
				{
					const auto old = *this;
					operator++();
					return old;
				}

			private:
				v_bool_iter _empty_iter;
				v_bool_iter _empty_end;
				v_obj_iter _obj_iter;
			};

			// returns a new game_obj in an indeterminate state
			game_obj* insert();
			// moves the passed game_obj into storage, then returns a ptr to it
			game_obj* insert(game_obj); 
			// marks the object as erased; ptrs to it will still be valid
			// use the difference between the object id and the ref id to
			// detect stale ptrs
			void erase(game_obj*) noexcept;
			// finds the non-erased object with that id, or returns nullptr
			const game_obj* find(entity_id) const noexcept; 
			game_obj* find(entity_id) noexcept;

			std::size_t size() const noexcept
			{
				return _size;
			}

			iterator begin() noexcept
			{
				return { std::begin(_emptys), std::end(_emptys), std::begin(_data) };
			}

			iterator end() noexcept
			{
				return { std::end(_emptys), std::end(_emptys), std::end(_data) };
			}

		private:
			std::size_t _size{};
			std::vector<bool> _emptys;
			std::deque<game_obj> _data;
		};

		static_assert(std::is_move_constructible_v<game_object_collection>);
	}

	//the whole game state, this is everything that gets saved
	// the only data not held here, is from the level file(terrain data)
	// and calculated state in extra_state
	struct game_state
	{
		//data_type = state_field<CurveType<T>>
		template<template<typename> typename CurveType, typename T>
		using data_type = state_field<CurveType<T>>;
		template<template<typename> typename CurveType, typename T>
		using data_colony = plf::colony<state_field<CurveType<T>>>;

		using state_data_type = detail::state_data_type_t<curve_types::type_pack>;
		state_data_type state_data;
		entity_id next_id = next(bad_entity);
		object_name_map names;
		std::unordered_map<entity_id, time_point> object_creation_time;
		std::unordered_map<entity_id, time_point> object_destruction_time;
		//TODO: pull stale objects out of the main game data, so that they can be written to disk or something
	};

	// non-saved state, this is generated during runtime from the actual game state.
	// this doesn't need to be sent to clients, they load or generate their own extras
	template<typename GameSystem>
	struct extra_state
	{
		detail::game_object_collection objects; 
		//system behaviours, including system local data
		system_behaviours<GameSystem> systems;
		//level local data, available in all systems
		any_map<unique_id> level_locals;
	};

	//functions for modifying game state
	namespace state_api
	{
		class game_state_error : public runtime_error
		{
		public:
			using runtime_error::runtime_error;
		};

		class object_id_collision : public game_state_error
		{
		public:
			using game_state_error::game_state_error;
		};

		class object_stale_error : public game_state_error
		{
		public:
			using game_state_error::game_state_error;
		};

		class object_property_not_found : public game_state_error
		{
		public:
			using game_state_error::game_state_error;
		};

		namespace loading
		{
			//restores an object from a save file, this can only be called before the game loop starts(during a load)
			//if an object is restored during a game then the object will not have the correct on_create/on_connect function called
			template<typename GameSystem>
			object_ref restore_object(const object_save_instance&, game_state&, extra_state<GameSystem>&);
		}

		namespace saving
		{
			//object_instance extract object
		}

		template<typename GameSystem>
		object_ref make_object(const object_instance&, time_point, game_state&, extra_state<GameSystem>&);
		// NOTE: the new object will not have the name of the cloned object
		template<typename GameSystem> 
		object_ref clone_object(const game_obj&, time_point, game_state&, extra_state<GameSystem>&);
		time_point get_object_creation_time(const game_obj&, const game_state&);
		// disconnects objects from their systems(also queues them to have on_disconnect called)
		template<typename GameSystem>
		void destroy_object(object_ref, time_point, game_state&, extra_state<GameSystem>&);
		// deletes the object data and invalidates the game_obj, best to do this one frame after detaching them.
		template<typename GameSystem>
		void erase_object(game_obj&, game_state&, extra_state<GameSystem>&);
		void name_object(string, object_ref, time_point, game_state&);
		template<typename GameSystem>
		object_ref get_object_ref(std::string_view, time_point, game_state&, extra_state<GameSystem>&) noexcept;
		// can throw object_stale_error
		// TODO: return game_obj&, throw stale_error
		template<typename GameSystem>
		game_obj& get_object(object_ref&, extra_state<GameSystem>&);
		// same as get_object except no LOGging
		template<typename GameSystem>
		game_obj* get_object_ptr(object_ref&, extra_state<GameSystem>&) noexcept;
		template<typename GameSystem>
		const game_obj* get_object_ptr(const object_ref&, const extra_state<GameSystem>&) noexcept;

		// NOTE: get_object_property_ref throws object_property_not_found if 
		//		 the requested variable is not stored in the object
		// CurveType is one of linear, step, pulse, const
		template<template<typename> typename CurveType, typename T>
		const CurveType<T>& get_object_property_ref(const game_obj&, variable_id);
		template<template<typename> typename CurveType, typename T>
		CurveType<T>& get_object_property_ref(game_obj&, variable_id);
		template<template<typename> typename CurveType, typename T>
		CurveType<T>* get_object_property_ptr(game_obj&, variable_id) noexcept;
	}
}

#include "hades/detail/game_state.inl"

#endif //!HADES_GAME_STATE_HPP
