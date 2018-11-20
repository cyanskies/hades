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

		if (g.input(label, std::get<float>(c.value)))
		{
			const auto rect = std::invoke(make_rect, o, c);
			const auto others = _quad.find_collisions(rect);

			const auto safe_pos = !std::any_of(std::begin(others), std::end(others), [rect, id = o.id](auto &&other){
				return intersects(rect, other.rect) && id != other.key;
			});

			//TODO: test safe_pos against map limits

			if (safe_pos || _allow_intersect)
			{
				std::invoke(apply, o, rect);
				_quad.insert(rect, o.id);
				update_object_sprite(o, _sprites);
			}
		}
	}
}