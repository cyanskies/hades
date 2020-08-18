#include "hades/game_api.hpp"

#include <type_traits>

#include "hades/data.hpp"
#include "hades/level_interface.hpp"

namespace hades
{
	namespace detail
	{
		template<typename T, typename GameSystem>
		T& get_level_local_ref_imp(unique_id id, extra_state<GameSystem>& extras)
		{
			auto val = extras.level_locals.try_get<T>(id);
			if (val) return *val;

			static_assert(std::is_default_constructible_v<T>);
			return extras.level_locals.set<T>(id, {});
		}
	}

	namespace game
	{
		template<typename T>
		T& get_system_data()
		{
			auto ptr = detail::get_game_data_ptr();
			assert(ptr->system_data->has_value());
			auto ret = std::any_cast<T>(ptr->system_data);
			assert(ret);
			return *ret;
		}

		template<typename T>
		void set_system_data(T value)
		{
			auto ptr = detail::get_game_data_ptr();
			ptr->system_data->emplace<std::decay_t<T>>(std::move(value));
		}
	}

	namespace game::mission
	{
		template<typename T>
		const game_property_curve<T>& get_curve(curve_index_t)
		{
			const auto ptr = detail::get_game_data_ptr();
			return detail::get_game_curve_ref(ptr->mission_data, i);
		}

		template<typename T>
		T get_value(curve_index_t i, time_point t)
		{
			const auto ptr = detail::get_game_data_ptr();
			const auto& c = get_curve<T>(i);
			return c.get(t);
		}

		template<typename T>
		T get_value(curve_index_t i)
		{
			return get_value<T>(i, get_last_time());
		}
	}

	namespace game::level
	{
		template<typename T>
		T& get_level_local_ref(unique_id id)
		{
			auto ptr = detail::get_game_level_ptr();
			return detail::get_level_local_ref_imp<T>(id, ptr->get_extras());
		}

		template<typename T>
		T get_property_value(object_ref o, variable_id v)
		{
			return get_property_ref<T>(o, v);
		}

		template<typename T>
		T& get_property_ref(object_ref o, variable_id v)
		{
			static_assert(curve_types::is_curve_type_v<T>);
			const auto g_ptr = detail::get_game_level_ptr();
			const auto o_ptr = state_api::get_object(o, g_ptr->get_extras());
			assert(o_ptr);
			// TODO: maybe handle null o_ptr due to stale ref
			return state_api::get_object_property_ref<T>(*o_ptr, v);
		}
		
		template<typename T>
		void set_property_value(object_ref o, variable_id v, T&& value)
		{
			auto& prop = get_property_ref<std::decay_t<T>>(o, v);
			prop = std::forward<T>(value);
		}
	}

	namespace render
	{
		template<typename T>
		T &get_system_data() noexcept
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr->system_data->has_value());
			auto ret = std::any_cast<T>(ptr->system_data);
			assert(ret);
			return *ret;
		}

		template<typename T>
		void set_system_data(T value)
		{
			auto ptr = detail::get_render_data_ptr();
			ptr->system_data->emplace<std::decay_t<T>>(std::move(value));
		}
	}

	namespace render::drawable
	{
		template<typename DrawableObject>
		id_t create(DrawableObject&& d, layer_t l)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);
			assert(ptr->render_output);
			return ptr->render_output->create_drawable_copy(std::forward<DrawableObject>(d), l);
		}

		template<typename DrawableObject>
		void update(id_t id, DrawableObject&& d, layer_t l)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);
			assert(ptr->render_output);
			ptr->render_output->update_drawable_copy(id, std::forward<DrawableObject>(d), l);
		}
	}

	namespace render::level
	{
		template<typename T>
		T& get_level_local_ref(unique_id id)
		{
			return detail::get_level_local_ref_imp<T>(id, *detail::get_render_extra_ptr());
		}

		template<typename T>
		T get_property_value(object_ref o, variable_id v)
		{
			return get_property_ref<T>(o, v);
		}

		template<typename T>
		T& get_property_ref(object_ref o, variable_id v)
		{
			static_assert(curve_types::is_curve_type_v<T>);
			const auto o_ptr = state_api::get_object(o, *detail::get_render_extra_ptr());
			assert(o_ptr);
			// TODO: maybe handle null o_ptr due to stale ref
			return state_api::get_object_property_ref<T>(*o_ptr, v);
		}
	}
}
