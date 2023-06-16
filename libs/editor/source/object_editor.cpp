#include "hades/object_editor.hpp"

#include "hades/core_curves.hpp"
#include "hades/gui.hpp"

namespace hades
{
	bool make_curve_default_value_editor(gui& g, std::string_view name,
		const resources::curve* curve, resources::curve_default_value &value,
		typename object_editor_ui_old<nullptr_t>::vector_curve_edit& target,
		typename object_editor_ui_old<nullptr_t>::cache_map& cache)
	{
		g.push_id(curve);
		const auto ret = std::visit([&g, &curve, &target, &cache, name](auto&& value)->bool {
			using Type = std::decay_t<decltype(value)>;
			if constexpr (!std::is_same_v<std::monostate, Type>)
			{
				auto nullr = nullptr_t{};
				if constexpr (resources::curve_types::is_collection_type_v<Type>)
					return detail::obj_ui::make_vector_property_edit<Type, nullptr_t, nullptr_t>(g, nullr, name, curve, value, target, cache);
				else
					return detail::obj_ui::make_property_edit<object_editor_ui_old<nullptr_t>>(g, nullr, name, *curve, value, cache);
			}
			else
				return false;
		}, value);
		g.pop_id(); // curve address

		return ret;
	}
}
