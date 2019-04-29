#include "hades/render_instance.hpp"

namespace hades 
{
	template<typename T>
	static void merge_input(const std::vector<exported_curves::export_set<T>>& input, curve_data& output)
	{
		auto& output_curves = get_curve_list<T>(output);
		for (const auto& [ent, var, frames] : input)
		{
			const auto id = std::decay_t<decltype(output_curves)>::key_type{ ent, var };
			const auto exists = output_curves.exists(id);
			auto new_curve = [exists, id, &output_curves]() {
				if (exists)
					return output_curves.get(id);
				else
				{
					const auto c = data::get<resources::curve>(id.second);
					return curve<T>{ c->c_type };
				}
			}();

			for (auto& f : frames)
				new_curve.set(f.t, f.value);

			if (exists)
				output_curves.set(id, new_curve);
			else
				output_curves.create(id, new_curve);
		}
	}

	void render_instance::input_updates(const exported_curves& input)
	{
		auto& curves = _game.get_curves();

		merge_input(input.int_curves, curves);
		merge_input(input.float_curves, curves);
		merge_input(input.bool_curves, curves);
		merge_input(input.string_curves, curves);
		merge_input(input.object_ref_curves, curves);
		merge_input(input.unique_curves, curves);

		merge_input(input.int_vector_curves, curves);
		merge_input(input.float_vector_curves, curves);
		merge_input(input.object_ref_vector_curves, curves);
		merge_input(input.unique_vector_curves, curves);
	}
}