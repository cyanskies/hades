#include "Objects/Objects.hpp"

#include <algorithm>

#include "Hades/Data.hpp"
#include "Hades/exceptions.hpp"

namespace objects
{
	using curve_obj = resources::object::curve_obj;

	//returns nullptr if the curve wasn't found
	//returns the curve with the value unset if it was found but not set
	//returns the requested curve plus it's value otherwise
	curve_obj TryGetCurve(const resources::object *o, const hades::resources::curve *c)
	{
		assert(o && c);

		using curve_t = hades::resources::curve;
		const curve_t *curve_ptr = nullptr;
		for (auto cur : o->curves)
		{
			auto curve = std::get<const curve_t*>(cur);
			if (curve == c)
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
			auto ret = TryGetCurve(obj, c);
			auto curve = std::get<const curve_t*>(ret);
			if (curve)
			{
				curve_ptr = curve;
				if (std::get<hades::resources::curve_default_value>(ret).set)
					return ret;
			}
		}

		return std::make_tuple(curve_ptr, hades::resources::curve_default_value());
	}

	curve_value ValidVectorCurve(hades::resources::curve_default_value v)
	{
		using vector_int = hades::resources::curve_types::vector_int;
		assert(std::holds_alternative<vector_int>(v.value));

		//the provided value is valid
		if (auto vector = std::get<vector_int>(v.value); v.set && vector.size() >= 2)
			return v;

		//invalid value, override with a minimum valid value
		v.set = true;
		v.value = vector_int{0, 0};

		return v;
	}

	curve_value GetCurve(const object_info &o, const hades::resources::curve *c)
	{
		assert(c);
		for (auto cur : o.curves)
		{
			auto curve = std::get<const hades::resources::curve*>(cur);
			auto v = std::get<hades::resources::curve_default_value>(cur);
			assert(curve);
			//if we have the right id and this
			if (curve == c && v.set)
				return v;
		}

		assert(o.obj_type);
		auto out = TryGetCurve(o.obj_type, c);
		if (auto[curve, value] = out; curve && value.set)
			return value;

		assert(c->default_value.set);
		return c->default_value;
	}

	curve_value GetCurve(const resources::object *o, const hades::resources::curve *c)
	{
		assert(o && c);

		auto out = TryGetCurve(o, c);
		using curve_t = hades::resources::curve;
		if (std::get<const curve_t*>(out) == nullptr)
			throw curve_not_found("Requested curve not found on object type: " + hades::data::GetAsString(o->id)
				+ ", curve was: " + hades::data::GetAsString(c->id));

		if (auto v = std::get<hades::resources::curve_default_value>(out); v.set)
			return v;

		assert(c->default_value.set);
		return c->default_value;
	}

	void SetCurve(object_info &o, const hades::resources::curve *c, curve_value v)
	{
		for (auto &curve : o.curves)
		{
			if (std::get<const hades::resources::curve*>(curve) == c)
			{
				std::get<curve_value>(curve) = std::move(v);
				return;
			}
		}

		o.curves.push_back({ c, v });
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
		auto last = std::end(list);
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
			} while (first != last && std::get<const curve*>(*first) == c);

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