#ifndef HADES_GAME_SYSTEM_HPP
#define HADES_GAME_SYSTEM_HPP

#include <any>
#include <deque>
#include <functional>
#include <vector>

#include "hades/curve_extra.hpp"
#include "hades/game_system_resources.hpp"
#include "hades/game_types.hpp"
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

namespace hades
{
	using system_data_t = std::any;

	/// @brief A range over the above object_time storage
	///			skips iterators to objects that haven't passed
	///			their activation timer yet
	class activated_object_view
	{
	public:
		constexpr activated_object_view() noexcept = default;
		activated_object_view(const name_list& n, time_point t) noexcept : _data{ &n },
			_activation_time{ t } {}

		template<typename T>
		class skip_iterator
		{
		public:
			skip_iterator(typename name_list::const_iterator i, const name_list& d, time_point t) noexcept :
				_i{ i }, _t{ t }, _data{ &d }
			{
				const auto end = std::end(*_data);
				while (_i != end && _i->next_activation > t) ++_i;
				return;
			}

			T& operator*() const noexcept
			{
				return _i->object;
			}

			T* operator->() const noexcept
			{
				return &_i->object;
			}

			skip_iterator& operator++() noexcept
			{
				const auto end = std::end(*_data);
				++_i;
				while (_i != end && _i->next_activation > _t) ++_i;
				return *this;
			}

			skip_iterator operator++(int) noexcept
			{
				const auto old = *this;
				operator++();
				return old;
			}

			bool operator==(const skip_iterator& other) const noexcept
			{
				return this->_i == other._i;
			}

			bool operator!=(const skip_iterator& other) const noexcept
			{
				return !(*this == other);
			}

		private:
			name_list::const_iterator _i;
			time_point _t;
			const name_list* _data;
		};

		using iterator = skip_iterator<object_ref>;
		using const_iterator = skip_iterator<const object_ref>;

		iterator begin() noexcept
		{
			assert(_data);
			return { _data->begin(), *_data, _activation_time };
		}

		iterator end() noexcept
		{
			assert(_data);
			return { _data->end(), *_data, _activation_time };
		}

		const_iterator begin() const noexcept
		{
			assert(_data);
			return { _data->begin(), *_data, _activation_time };
		}

		const_iterator end() const noexcept
		{
			assert(_data);
			return { _data->end(), *_data, _activation_time };
		}

	private:
		const name_list* _data = nullptr;
		time_point _activation_time;
	};

	template<typename SystemType>
	class system_behaviours
	{
	public:
		using system_type = SystemType;
		using system_resource = typename SystemType::system_t;

		std::vector<SystemType*> get_systems() noexcept
		{
			auto out = std::vector<SystemType*>{};
			out.reserve(size(_systems));
			std::transform(begin(_systems), end(_systems), std::back_inserter(out), [](auto& sys) {
				return &sys;
				});

			return out;
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

		[[deprecated]]
		void set_attached(unique_id, name_list);

		// call if time scrubbing, will set the new and removed ents
		// correctly
		[[deprecated]]
		void set_current_time(time_point);

		//get entites that have been added to the system
		//since the last frame
		name_list get_new_entities(SystemType&);
		// entities that have already been attached in a previous session
		// but are being reinitialised in on_create
		name_list get_created_entities(SystemType&);
		//get all entities currently attached to the system
		const name_list& get_entities(SystemType&) const;
		//get entities that were removed from the system last frame
		name_list get_removed_entities(SystemType&);

		void attach_system(object_ref, unique_id);
		void attach_system_from_load(object_ref, unique_id);
		[[deprecated]] void detach_system(object_ref, unique_id);
		//remove this entity from all systems
		void detach_all(object_ref);

		//this entity won't trigger on_tick events untill the provided time point
		void sleep_entity(object_ref, unique_id, time_point);

		bool needs_update() noexcept
		{
			return std::exchange(_dirty_systems, false);
		}

	private:
		std::deque<SystemType> _systems;
		std::vector<const system_resource*> _new_systems;
		std::unordered_map<unique_id, system_data_t> _system_data;
		bool _dirty_systems = false;
	};

	//fwd
	class game_interface;
	using game_system = detail::basic_system<resources::system>;
	struct player_data;

	template<typename T>
	struct extra_state;

	template<typename SystemType>
	struct common_job_data
	{
		using system_type = SystemType;

		time_point current_time;
		extra_state<SystemType>* extra = nullptr;
		system_behaviours<SystemType>* systems = nullptr;
		// the following member may change between invocation
		activated_object_view entity;
		unique_id system = unique_zero;
		system_data_t* system_data = nullptr;
	};

	struct system_job_data : common_job_data<game_system>
	{
		using get_level_fn = game_interface*(*)(unique_id);

		get_level_fn get_level = nullptr;
		//level data interface:
		// contains units, particles, buildings, terrain
		// per level quests and objectives
		game_interface *level_data = nullptr; // <- currently open level
		//mission data interface
		game_interface *mission_data = nullptr;
		const std::vector<player_data>* players = nullptr;
		time_duration dt = time_duration::zero();
	};

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	const resources::system* make_system(unique_id id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager&);

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	const resources::system* make_system(std::string_view name, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &data)
	{
		return make_system(data.get_uid(name), on_create, on_connect, on_disconnect, on_tick, on_destroy, data);
	}

	//program provided systems should be attatched to the renderer or 
	//gameinstance depending on what kind of system they are

	//scripted systems should be listed in the game_system: and render_system: lists in
	//the mod files that added them

	class render_interface;
	using render_system = detail::basic_system<resources::render_system>;
	class common_interface;
	
	struct render_job_data : common_job_data<render_system>
	{
		const common_interface *level_data = nullptr; //TODO: client_interface to lock down access
		render_interface *render_output = nullptr;
	};

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	const resources::render_system* make_render_system(unique_id id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager&);
	
	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	const resources::render_system* make_render_system(std::string_view id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &d)
	{
		return make_render_system(d.get_uid(id), on_create, on_connect, on_disconnect, on_tick, on_destroy, d);
	}

	//funcs to call before a system gets control, and to clean up after
	void set_game_data(system_job_data*) noexcept;
	void set_render_data(render_job_data*) noexcept;

	namespace detail
	{
		//TODO: functions to swap current level
		//		these would be for examining state of another area
		system_job_data* get_game_data_ptr() noexcept;
		game_interface* get_game_level_ptr() noexcept;
		system_behaviours<game_system>* get_game_systems_ptr() noexcept;
		void change_level(game_interface*) noexcept;
		render_job_data* get_render_data_ptr() noexcept;
		const common_interface* get_render_level_ptr() noexcept;
		extra_state<render_system>* get_render_extra_ptr() noexcept;
	}
}

#include "hades/detail/game_system.inl"

#endif //!HADES_GAMESYSTEM_HPP
