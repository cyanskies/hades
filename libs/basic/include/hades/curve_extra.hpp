#ifndef HADES_CURVE_EXTRA_HPP
#define HADES_CURVE_EXTRA_HPP

// TODO: rename to curve_resource.hpp

#include <type_traits>
#include <variant>
#include <vector>

#include "hades/curve.hpp"
#include "hades/curve_types.hpp"
#include "hades/data.hpp"
#include "hades/exceptions.hpp"
//extra functions and struct for loading curves as a resource

//forwards
namespace hades::data
{
	class parser_node;
}

namespace hades
{
	void register_curve_resource(data::data_manager&);

	class invalid_curve : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};
}

namespace hades::resources
{
	using curve_variable_type = hades::curve_variable_type;
	namespace curve_types = hades::curve_types;

	namespace detail
	{
		template<typename T> struct curve_default_value_type;
		template<typename... Ts>
		struct curve_default_value_type<std::tuple<Ts...>>
		{
			using type = std::variant<std::monostate, Ts...>;
		};
		template<typename T>
		using curve_default_value_t = typename curve_default_value_type<T>::type;
	}

	using curve_default_value = detail::curve_default_value_t<curve_types::type_pack>;

	static_assert(std::is_move_constructible_v<curve_default_value>);

	struct curve_t {};

	struct curve : public resource_type<curve_t>
	{
		void serialise(const data::data_manager&, data::writer&) const final override;

		curve_variable_type data_type = curve_variable_type::error;
        keyframe_style frame_style = keyframe_style::default_value;
		bool sync = false; //sync shares the value with clients
		bool locked = false; // sets whether the curve is locked from level editor
		bool hidden = false; // don't show the curve in level editor
		
		curve_default_value default_value{};
	};

	[[nodiscard]] curve_default_value reset_default_value(const curve &c);

	constexpr bool is_set(const curve_default_value& v) noexcept
	{
		return !std::holds_alternative<std::monostate>(v);
	}
	
	bool is_curve_valid(const resources::curve&) noexcept;
	bool is_curve_valid(const resources::curve&, const curve_default_value&) noexcept;

	curve_default_value curve_from_str(data::data_manager&, const resources::curve&, const std::variant<std::monostate, string, std::vector<string>>&);
	curve_default_value curve_from_node(const resources::curve&, const data::parser_node&);

	template<typename T>
	const curve* make_curve(data::data_manager&, unique_id name, curve_variable_type, keyframe_style, T default_value, bool sync, bool locked = false, bool hidden = false);
	template<typename T>
	const curve* make_curve(data::data_manager&, std::string_view name, curve_variable_type, keyframe_style, T default_value, bool sync, bool locked = false, bool hidden = false);

	const std::vector<const curve*> &get_all_curves();
}

namespace hades
{
	template<template<typename> typename CurveType, typename VariableType>
	consteval std::pair<keyframe_style, curve_variable_type> get_curve_info() noexcept;
	string to_string(keyframe_style);
	string to_string(std::pair<keyframe_style, curve_variable_type>);

	string to_string(const resources::curve &c);
	string to_string(std::tuple<const resources::curve&, const resources::curve_default_value&> curve);
	string curve_to_string(const resources::curve &c, const resources::curve_default_value &v);
}

#include "hades/detail/curve_extra.inl"

#endif // !HADES_CURVE_EXTRA_HPP
