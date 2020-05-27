#include "hades/curve_extra.hpp"

namespace hades::resources
{
	namespace detail
	{
		void add_to_curve_master_list(const curve*);
	}

	template<typename T>
	const curve* make_curve(data::data_manager& d, unique_id name, curve_variable_type v_type, T default_value, bool sync, bool save)
	{
		auto c = d.find_or_create<curve>(name, unique_id::zero);
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
		curve_variable_type v_type, T default_value, bool sync,
		bool save)
	{
		const auto id = d.get_uid(name);
		return make_curve(d, id, v_type, std::move(default_value), sync, save);
	}
}