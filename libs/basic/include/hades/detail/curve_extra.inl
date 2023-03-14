#include "hades/curve_extra.hpp"

#include "hades/tuple.hpp"

namespace hades::resources
{
	namespace detail
	{
		void add_to_curve_master_list(const curve*);
	}

	template<typename T>
	const curve* make_curve(data::data_manager& d, unique_id name, curve_variable_type v_type, keyframe_style k_style, T default_value, bool sync, bool locked, bool hidden)
	{
		using namespace std::string_view_literals;
		auto c = d.find_or_create<curve>(name, {}, "curves"sv);
		c->data_type = v_type;
        c->frame_style = k_style;
		c->default_value = std::move(default_value);
		c->sync = sync;
		c->locked = locked;
		c->hidden = hidden;
		c->loaded = true;

		assert(is_curve_valid(*c));
		detail::add_to_curve_master_list(c);
		return c;
	}

	template<typename T>
	const curve* make_curve(data::data_manager& d, std::string_view name,
		curve_variable_type v_type, keyframe_style k_style, T default_value, bool sync,
		bool locked, bool hidden)
	{
		const auto id = d.get_uid(name);
		return make_curve(d, id, v_type, k_style, std::move(default_value), sync, locked, hidden);
	}
}

namespace hades
{
	template<template<typename> typename CurveType, typename VariableType>
	consteval std::pair<keyframe_style, curve_variable_type> get_curve_info() noexcept
	{
		static_assert(is_const_curve_v<CurveType<VariableType>> ||
			is_linear_curve_v<CurveType<VariableType>> ||
			is_pulse_curve_v<CurveType<VariableType>> ||
			is_step_curve_v<CurveType<VariableType>>,
			"CurveType isn't one of the types provided in curves.hpp");
		static_assert(curve_types::is_curve_type_v<VariableType>);

		auto keyframe = keyframe_style::const_t;
		if constexpr (is_linear_curve_v<CurveType<VariableType>>)
			keyframe = keyframe_style::linear;
		else if constexpr (is_pulse_curve_v<CurveType<VariableType>>)
			keyframe = keyframe_style::pulse;
		else if constexpr (is_step_curve_v<CurveType<VariableType>>)
			keyframe = keyframe_style::step;

		auto var = curve_variable_type::begin;
		tuple_for_each(curve_types::type_pack{}, [&var](const auto& type, const auto index) noexcept {
			using T = std::decay_t<decltype(type)>;
			if constexpr (std::is_same_v<T, VariableType>)
				var = curve_variable_type{ static_cast<std::underlying_type_t<curve_variable_type>>(index) };
			return;
		});

		return { keyframe, var };
	}
}
