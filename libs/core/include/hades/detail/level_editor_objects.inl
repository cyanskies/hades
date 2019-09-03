#include "hades/level_editor_objects.hpp"

namespace hades
{
	template<typename MakeBoundRect, typename SetChangedProperty>
	inline void hades::level_editor_objects::_make_positional_property_edit_field(gui &g, std::string_view label,
		editor_object_instance &o, curve_info &c, MakeBoundRect make_rect, SetChangedProperty apply)
	{
		static_assert(std::is_invocable_r_v<rect_float, MakeBoundRect, const object_instance&, const level_editor_objects::curve_info&>,
			"MakeBoundRect must have the following definition: (const object_instance&, const curve_info&)->rect_float");
		static_assert(std::is_invocable_v<SetChangedProperty, object_instance&, const rect_float&>,
			"SetChangedProperty must have the following definition (object_instance&, const rect_float&)");

		const auto v = std::get<vector_float>(c.value);
		auto value = std::array{ v.x, v.y };

		if (g.input(label, value))
		{
			c.value = vector_float{ value[0], value[1] };
			//TODO: must use both collision systems here
			const auto rect = std::invoke(make_rect, o, c);
			const auto safe_pos = _object_valid_location(position(rect), size(rect), o);

			//TODO: test safe_pos against map limits

			if (safe_pos || _allow_intersect)
			{
				std::invoke(apply, o, rect);
				_update_quad_data(o);
				update_object_sprite(o, _sprites);
			}
		}
	}
}