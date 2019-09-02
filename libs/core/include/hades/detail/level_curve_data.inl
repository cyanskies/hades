#include "hades/level_curve_data.hpp"

namespace hades::detail
{
	template<typename T>
	inline curve_data::curve_map<T>& get_curve_list(curve_data& data)
	{
		assert(false);
		throw std::logic_error{ "invalid type" };
	}

	template<>
	inline curve_data::curve_map<resources::curve_types::int_t>& get_curve_list(curve_data& data) noexcept
	{
		return data.int_curves;
	}

	template<>
	inline curve_data::curve_map<resources::curve_types::float_t>& get_curve_list(curve_data& data) noexcept
	{
		return data.float_curves;
	}

	template<>
	inline curve_data::curve_map<resources::curve_types::vec2_float>& get_curve_list(curve_data& data) noexcept
	{
		return data.vec2_float_curves;
	}

	template<>
	inline curve_data::curve_map<resources::curve_types::bool_t>& get_curve_list(curve_data& data) noexcept
	{
		return data.bool_curves;
	}

	template<>
	inline curve_data::curve_map<resources::curve_types::string>& get_curve_list(curve_data& data) noexcept
	{
		return data.string_curves;
	}

	template<>
	inline curve_data::curve_map<resources::curve_types::object_ref>& get_curve_list(curve_data& data) noexcept
	{
		return data.object_ref_curves;
	}

	template<>
	inline curve_data::curve_map<resources::curve_types::unique>& get_curve_list(curve_data& data) noexcept
	{
		return data.unique_curves;
	}

	template<>
	inline curve_data::curve_map<resources::curve_types::collection_int>& get_curve_list(curve_data& data) noexcept
	{
		return data.int_vector_curves;
	}

	template<>
	inline curve_data::curve_map<resources::curve_types::collection_float>& get_curve_list(curve_data& data) noexcept
	{
		return data.float_vector_curves;
	}

	template<>
	inline curve_data::curve_map<resources::curve_types::collection_object_ref>& get_curve_list(curve_data& data) noexcept
	{
		return data.object_ref_vector_curves;
	}

	template<>
	inline curve_data::curve_map<resources::curve_types::collection_unique>& get_curve_list(curve_data& data) noexcept
	{
		return data.unique_vector_curves;
	}
}

namespace hades
{
	template<typename T>
	inline curve_data::curve_map<std::decay_t<T>>& get_curve_list(curve_data& data)
		noexcept(resources::curve_types::is_curve_type_v<std::decay_t<T>>)
	{
		return detail::get_curve_list<std::decay_t<T>>(data);
	}

	template<typename T>
	inline const curve_data::curve_map<std::decay_t<T>>& get_curve_list(const curve_data& data)
		noexcept(resources::curve_types::is_curve_type_v<std::decay_t<T>>)
	{
		//NOTE: we return a const ref
		auto &unconst_data = const_cast<curve_data&>(data);
		return get_curve_list<T>(unconst_data);
	}
}
