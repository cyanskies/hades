#include "Objects/Objects.hpp"

namespace objects
{
	resources::object::curve_list::value_type GetCurve(const object_info &o, hades::data::UniqueId c)
	{

	}

	resources::object::curve_list::value_type GetCurve(const resources::object *o, hades::data::UniqueId c)
	{
		assert(o);
	}

	resources::object::curve_list GetAllCurves(const object_info &o)
	{

	}

	resources::object::curve_list GetAllCurves(const resources::object *o)
	{
		assert(o);
	}

	const hades::resources::animation *GetEditorIcon(const resources::object *o)
	{
		assert(o);
		if (o->editor_icon)
			return o->editor_icon;

		if (!o->base.empty())
		{
			for (auto &b : o->base)
			{
				auto icon = GetEditorIcon(b);
				if (icon)
					return icon;
			}
		}

		return nullptr;
	}

	resources::object::animation_list GetEditorAnimations(const resources::object *o)
	{
		assert(o);
		if (!o->editor_anims.empty())
			return o->editor_anims;

		if (!o->base.empty())
		{
			for (auto &b : o->base)
			{
				auto anim = GetEditorAnimations(b);
				if (!anim.empty())
					return anim;
			}
		}

		return resources::object::animation_list();
	}
}