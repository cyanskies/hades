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

		template<typename T, typename GameSystem>
		void set_level_local_value_imp(unique_id id, T value, extra_state<GameSystem>& extras)
		{
			extras.level_locals.set(id, std::move(value));
			return;
		}
	}

	namespace game
	{
		namespace detail = hades::detail;
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
		void set_level_local_value(unique_id id, T value)
		{
			detail::set_level_local_value_imp<T>(id, std::move(value));
			return;
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
			auto& obj = state_api::get_object(o, g_ptr->get_extras());
			return state_api::get_object_property_ref<T>(obj, v);
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
		namespace detail = hades::detail;
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
		void set_level_local_value(unique_id id, T value)
		{
			return detail::set_level_local_value_imp(id, std::move(value), *detail::get_render_extra_ptr());
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
			auto& obj = state_api::get_object(o, *detail::get_render_extra_ptr());
			return state_api::get_object_property_ref<T>(obj, v);
		}
	}
}
