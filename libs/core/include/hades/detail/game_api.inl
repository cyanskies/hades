#include "hades/game_api.hpp"

#include <type_traits>

#include "hades/data.hpp"
#include "hades/level_interface.hpp"

namespace hades
{
	namespace detail
	{
		template<typename T>
		T& get_level_local_ref_imp(unique_id id, common_interface* ptr)
		{
			static_assert(std::is_default_constructible_v<T>);
			auto& any = ptr->get_level_local_ref(id);
			if (!any.has_value())
				any.emplace<T>();

			return *std::any_cast<T>(&any);
		}

		template<typename T>
		curve<T> &get_game_curve_ref(common_interface* l, curve_index_t i)
		{
			auto& curves = l->get_curves();
			auto& curve_map = hades::get_curve_list<T>(curves);
			auto iter = curve_map.find(i);
			if (iter == end(curve_map))
				throw curve_not_found{"unable to find the requested curve on this object: " + to_string(i.second)};
			return iter->second;
		}

		template<typename T>
		const hades::game::curve_keyframe<T> *get_game_curve_frame_ref(const common_interface* l, curve_index_t i, time_point t)
		{
			const auto& curves = l->get_curves();
			const auto& curve_map = hades::get_curve_list<T>(curves);
			const auto curve_iter = curve_map.find(i);
			assert(curve_iter != end(curve_map));
			return curve_iter->second.getPrevious(t);
		}

		template<typename T>
		const hades::game::curve_keyframe<T>* get_game_curve_frame_after_ref(const common_interface* l, curve_index_t i, time_point t)
		{
			const auto& curves = l->get_curves();
			const auto& curve_map = hades::get_curve_list<T>(curves);
			const auto curve_iter = curve_map.find(i);
			assert(curve_iter != end(curve_map));
			return curve_iter->second.getNext(t);
		}

		template<typename T>
		std::pair<const hades::game::curve_keyframe<T>*, const hades::game::curve_keyframe<T>*> get_game_curve_frame_pair_ref(const common_interface* l, curve_index_t i, time_point t)
		{
			const auto& curves = l->get_curves();
			const auto& curve_map = hades::get_curve_list<T>(curves);
			const auto curve_iter = curve_map.find(i);
			assert(curve_iter != end(curve_map));
			return curve_iter->second.get_range(t);
		}

		template<typename T>
		void set_game_curve(common_interface*l, curve_index_t i, curve<T> c)
		{
			auto& curves = l->get_curves();
			auto& target_curve_list = hades::get_curve_list<T>(curves);
			target_curve_list.insert_or_assign(i, std::move(c));
			return;
		}

		template<typename T>
		void set_game_value(common_interface*l, curve_index_t i, time_point t, T&& v)
		{
			auto& curves = l->get_curves();
			auto& target_curve_type = hades::get_curve_list<T>(curves);
			auto iter = target_curve_type.find(i);

			if (iter == std::end(target_curve_type))
			{
				//curve doesn't exist
				const auto obj_id = to_string(to_value(i.first));
				const auto curve = to_string(i.second);
				LOGERROR("Tried to set missing curve, curve was: " + curve + ", object was: " + obj_id);
				return;
			}

			auto &c = iter->second;
			c.set(t, std::forward<T>(v));
			return;
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
		const curve<T>& get_curve(curve_index_t)
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
			return detail::get_level_local_ref_imp<T>(id, ptr);
		}

		template<typename T>
		const curve<T> &get_curve(curve_index_t i)
		{
			const auto ptr = detail::get_game_level_ptr();
			return detail::get_game_curve_ref<T>(ptr, i);
		}

		template<typename T>
		const curve_keyframe<T>* get_keyframe_ref(curve_index_t i, time_point t)
		{
			const auto ptr = detail::get_game_data_ptr();
			return detail::get_game_curve_frame_ref<T>(ptr->level_data, i, t);
		}

		template<typename T>
		optional_keyframe<T> get_keyframe(curve_index_t i, time_point t)
		{
			return get_keyframe_ref<T>(i, t);
		}

		template<typename T>
		optional_keyframe<T> get_keyframe(curve_index_t index)
		{
			return get_keyframe<T>(index, get_last_time());
		}

		template<typename T>
		const curve_keyframe<T>* get_keyframe_after_ref(curve_index_t i, time_point t)
		{
			const auto ptr = detail::get_game_data_ptr();
			return detail::get_game_curve_frame_after_ref<T>(ptr->level_data, i, t);
		}

		template<typename T>
		optional_keyframe<T> get_keyframe_after(curve_index_t i, time_point t)
		{
			return get_keyframe_after_ref<T>(i, t);
		}

		template<typename T>
		optional_keyframe<T> get_keyframe_after(curve_index_t i)
		{
			return get_keyframe_after_ref<T>(i, get_last_time());
		}

		template<typename T>
		std::pair<const curve_keyframe<T>*, const curve_keyframe<T>*> get_keyframe_pair_ref(curve_index_t i, time_point t)
		{
			const auto ptr = detail::get_game_level_ptr();
			return detail::get_game_curve_frame_pair_ref<T>(ptr, i, t);
		}

		template<typename T>
		std::pair<optional_keyframe<T>, optional_keyframe<T>> get_keyframe_pair(curve_index_t i, time_point t)
		{
			const auto p = get_keyframe_pair_ref<T>(i, t);

			optional_keyframe<T> first{}, second{};
			if (p.first)
				first.emplace(*p.first);
			if (p.second)
				second.emplace(*p.second);

			return { first, second };
		}

		template<typename T>
		std::pair<optional_keyframe<T>, optional_keyframe<T>> get_keyframe_pair(curve_index_t i)
		{
			return get_keyframe_pair<T>(i, get_last_time());
		}

		template<typename T>
		const T& get_ref(curve_index_t i, time_point t)
		{
			const auto &curve = get_curve<T>(i);
			return curve.get_ref(t);
		}

		template<typename T>
		const T& get_ref(curve_index_t i)
		{
			return get_ref<T>(i, get_last_time());
		}

		template<typename T>
		T get_value(curve_index_t i, time_point t)
		{
			const auto& curve = get_curve<T>(i);
			return curve.get(t);
		}

		template<typename T>
		T get_value(curve_index_t i)
		{
			return get_value<T>(i, get_last_time());
		}

		template<typename T>
		void set_curve(curve_index_t i, curve<T> c)
		{
			auto ptr = detail::get_game_data_ptr();
			return detail::set_game_curve(ptr->level_data, i, std::move(c));
		}

		template<typename T>
		void set_value(curve_index_t i, time_point t, T&& v)
		{
			auto ptr = detail::get_game_data_ptr();
			return detail::set_game_value(ptr->level_data, i, t, std::forward<T>(v));
		}

		template<typename T>
		void set_value(curve_index_t i, T&& t)
		{
			return set_value(i, get_time(), std::forward<T>(t));
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
			auto ptr = detail::get_render_level_ptr();
			return detail::get_level_local_ref_imp<T>(id, ptr);
		}

		template<typename T>
		const curve<T>& get_curve(curve_index_t i)
		{
			const auto ptr = detail::get_render_data_ptr();
			return detail::get_game_curve_ref<T>(ptr->level_data, i);
		}

		template<typename T>
		const T& get_ref(curve_index_t, time_point)
		{
			const auto &curve = get_curve<T>(i);
			return curve.get_ref(t);
		}

		template<typename T>
		const T& get_ref(curve_index_t i)
		{
			return get_ref(i, get_time());
		}

		template<typename T>
		const T get_value(curve_index_t i, time_point t)
		{
			const auto &curve = get_curve<T>(i);
			return curve.get(t);
		}

		template<typename T>
		const T get_value(curve_index_t i)
		{
			return get_value<T>(i, get_time());
		}
	}
}