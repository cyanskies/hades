#include "hades/curve_extra.hpp"

namespace hades::resources::curve_types
{
	template<typename T, std::enable_if_t<is_curve_type_v<T>, int>>
	string curve_type_to_string() noexcept
	{
		using namespace std::string_literals;
		if constexpr (std::is_same_v<T, int_t>)
			return "int32"s;
		else if constexpr (std::is_same_v<T, float_t>)
			return "float"s;
		else if constexpr (std::is_same_v<T, vec2_float>)
			return "vec2_float"s;
		else if constexpr (std::is_same_v<T, bool_t>)
			return "bool"s;
		else if constexpr (std::is_same_v<T, string>)
			return "string"s;
		else if constexpr (std::is_same_v<T, object_ref>)
			return "obj_ref"s;
		else if constexpr (std::is_same_v<T, unique>)
			return "unique"s;
		else if constexpr (std::is_same_v<T, collection_int>)
			return "collection_int32"s;
		else if constexpr (std::is_same_v<T, collection_float>)
			return "collection_float"s;
		else if constexpr (std::is_same_v<T, collection_object_ref>)
			return "collection_obj_ref"s;
		else if constexpr (std::is_same_v<T, collection_unique>)
			return "collection_unique"s;
	}
}

namespace hades::resources
{
	namespace detail
	{
		void add_to_curve_master_list(const curve*);
	}

	template<typename T>
	const curve* make_curve(data::data_manager& d, unique_id name, curve_type c_type, curve_variable_type v_type, T default_value, bool sync, bool save)
	{
		auto c = d.find_or_create<curve>(name, unique_id::zero);
		c->c_type = c_type;
		c->data_type = v_type;
		c->default_value = std::move(default_value);
		c->sync = sync;
		c->save = save;
		c->loaded = true;

		assert(is_curve_valid(*c));
		detail::add_to_curve_master_list(c);
		return c;
	}

	template<typename T>
	const curve* make_curve(data::data_manager& d, std::string_view name,
		curve_type c_type, curve_variable_type v_type, T default_value, bool sync,
		bool save)
	{
		const auto id = d.get_uid(name);
		return make_curve(d, id, c_type, v_type, std::move(default_value), sync, save);
	}
}