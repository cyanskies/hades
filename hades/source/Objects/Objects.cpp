#include "Objects/Objects.hpp"

#include <algorithm>

namespace objects
{
	curve_obj GetCurve(const object_info &o, hades::data::UniqueId c)
	{
		for (auto cur : o.curves)
		{
			auto curve = std::get<const hades::resources::curve*>(cur);
			auto v = std::get<hades::resources::curve_default_value>(cur);
			assert(curve);
			//if we have the right id and this
			if (curve->id == c && v.set)
				return cur;
		}

		assert(o.obj_type);
		return GetCurve(o.obj_type, c);
	}

	curve_obj GetCurve(const resources::object *o, hades::data::UniqueId c)
	{
		assert(o);
		
		using curve_t = hades::resources::curve;
		const curve_t *curve_ptr = nullptr;
		for (auto cur : o->curves)
		{	
			auto curve = std::get<const curve_t*>(cur);
			if (curve->id == c)
			{
				curve_ptr = curve;
				auto v = std::get<hades::resources::curve_default_value>(cur);
				if (v.set)
					return cur;
			}
		}

		for (auto obj : o->base)
		{
			assert(obj);
			auto ret = GetCurve(obj, c);
			if (std::get<const curve_t*>(ret))
				return ret;
		}

		return std::make_tuple(curve_ptr, hades::resources::curve_default_value());
	}

	curve_list UniqueCurves(curve_list list)
	{
		//list should not contain any nullptr curves
		using curve = hades::resources::curve;
		using value = hades::resources::curve_default_value;

		//place all curves of the same type adjacent to one another
		std::stable_sort(std::begin(list), std::end(list), [](const curve_obj &lhs, const curve_obj &rhs) {
			auto c1 = std::get<const curve*>(lhs), c2 = std::get<const curve*>(rhs);
			assert(c1 && c2);
			return c1->id < c2->id;
		});

		//remove any adjacent duplicates(for curves with the same type and value)
		std::unique(std::begin(list), std::end(list), [](const curve_obj &lhs, const curve_obj &rhs) {
			auto c1 = std::get<const curve*>(lhs), c2 = std::get<const curve*>(rhs);
			auto v1 = std::get<value>(lhs), v2 = std::get<value>(rhs);
			assert(c1 && c2);
			return c1->id == c2->id
				&& v1 == v2;
		});

		curve_list output;

		//for each unique curve, we want to keep the 
		//first with an assigned value or the last if their is no value
		auto first = std::begin(list);
		auto last = std::begin(list);
		while (first != last)
		{
			value v;
			auto c = std::get<const curve*>(*first);
			//while each item represents the same curve c
			//store the value if it is set
			//and then find the next different curve
			// or iterate to the next c
			do {
				auto val = std::get<value>(*first);
				if (val.set)
				{
					v = val;
					first = std::find_if_not(first, last, [c](const curve_obj &lhs) {
						return c == std::get<const curve*>(lhs);
					});
				}
				else
					++first;
			} while (std::get<const curve*>(*first) == c);

			//store c and v in output
			output.push_back(std::make_tuple(c, v));
		}

		return output;
	}

	curve_list GetAllCurvesSimple(const resources::object *o)
	{
		assert(o);
		curve_list out;

		for (auto c : o->curves)
			out.push_back(c);

		for (auto obj : o->base)
		{
			assert(obj);
			auto curves = GetAllCurvesSimple(obj);
			std::move(std::begin(curves), std::end(curves), std::back_inserter(out));
		}

		return out;
	}

	curve_list GetAllCurves(const object_info &o)
	{
		curve_list output;

		for (auto c : o.curves)
			output.push_back(c);

		assert(o.obj_type);
		auto inherited_curves = GetAllCurvesSimple(o.obj_type);
		std::move(std::begin(inherited_curves), std::end(inherited_curves), std::back_inserter(output));

		return UniqueCurves(output);
	}
	
	curve_list GetAllCurves(const resources::object *o)
	{
		assert(o);
		auto curves = GetAllCurvesSimple(o);
		return UniqueCurves(curves);
	}

	const hades::resources::animation *GetEditorIcon(const resources::object *o)
	{
		assert(o);
		if (o->editor_icon)
			return o->editor_icon;

		for (auto b : o->base)
		{
			assert(b);
			auto icon = GetEditorIcon(b);
			if (icon)
				return icon;
		}

		return nullptr;
	}

	resources::object::animation_list GetEditorAnimations(const resources::object *o)
	{
		assert(o);
		if (!o->editor_anims.empty())
			return o->editor_anims;

		for (auto b : o->base)
		{
			assert(b);
			auto anim = GetEditorAnimations(b);
			if (!anim.empty())
				return anim;
		}

		return resources::object::animation_list();
	}
}