#ifndef HADES_CURVE_EXTRA_HPP
#define HADES_CURVE_EXTRA_HPP

#include <type_traits>
#include <variant>
#include <vector>

#include "hades/curve.hpp"
#include "hades/curve_types.hpp"
#include "hades/data.hpp"
#include "hades/entity_id.hpp"
#include "hades/types.hpp"
#include "hades/uniqueid.hpp"
#include "hades/vector_math.hpp"

//extra functions and struct for loading curves as a resource

namespace hades
{
	void register_curve_resource(data::data_manager&);

	class invalid_curve : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
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
		curve_variable_type data_type = curve_variable_type::error;
		keyframe_style keyframe_style = keyframe_style::const_t;
		bool sync = false, //sync shares the value with clients
			save = false; //save records the value in save files
		// TODO: is non-saved curves ever a good idea???
		curve_default_value default_value{};
	};

	[[nodiscard]] curve_default_value reset_default_value(const curve &c);

	constexpr bool is_set(const curve_default_value& v) noexcept
	{
		return !std::holds_alternative<std::monostate>(v);
	}
	
	bool is_curve_valid(const resources::curve&) noexcept;
	bool is_curve_valid(const resources::curve&, const curve_default_value&) noexcept;

	curve_default_value curve_from_node(const resources::curve&, const data::parser_node&);

	template<typename T>
	const curve* make_curve(data::data_manager&, unique_id name, curve_variable_type, keyframe_style, T default_value, bool sync, bool save);
	template<typename T>
	const curve* make_curve(data::data_manager&, std::string_view name, curve_variable_type, keyframe_style, T default_value, bool sync, bool save);

	const std::vector<const curve*> &get_all_curves();
}

namespace hades
{
	string to_string(const resources::curve &c);
	string to_string(std::tuple<const resources::curve&, const resources::curve_default_value&> curve);

	string curve_to_string(const resources::curve &c, const resources::curve_default_value &v);
}

#include "hades/detail/curve_extra.inl"

#endif // !HADES_CURVE_EXTRA_HPP
