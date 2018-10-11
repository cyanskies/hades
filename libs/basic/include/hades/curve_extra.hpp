#ifndef HADES_CURVE_EXTRA_HPP
#define HADES_CURVE_EXTRA_HPP

#include <type_traits>
#include <variant>
#include <vector>

#include "hades/curve.hpp"
#include "hades/data.hpp"
#include "hades/game_types.hpp"
#include "hades/types.hpp"
#include "hades/uniqueid.hpp"

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
	enum class curve_variable_type { error, int_t, float_t, bool_t, string, object_ref, unique, vector_int, vector_float, vector_object_ref, vector_unique };

	namespace curve_types
	{
		using int_t = hades::types::int32;
		using float_t = float;
		using bool_t = bool;
		using string = hades::types::string;
		using object_ref = entity_id;
		using unique = hades::unique_id;
		using vector_int = std::vector<int_t>;
		using vector_float = std::vector<float_t>;
		using vector_object_ref = std::vector<object_ref>;
		using vector_unique = std::vector<unique>;

		template <typename T>
		struct is_vector_type : public std::false_type {};

		template <>
		struct is_vector_type<vector_int> : public std::true_type {};

		template <>
		struct is_vector_type<vector_float> : public std::true_type {};

		template <>
		struct is_vector_type<vector_object_ref> : public std::true_type {};

		template <>
		struct is_vector_type<vector_unique> : public std::true_type {};

		template<typename T>
		constexpr auto is_vector_type_v = is_vector_type<T>::value;
	}

	using curve_default_value = std::variant<
		std::monostate,
		curve_types::int_t, 
		curve_types::float_t,
		curve_types::bool_t, 
		curve_types::string, 
		curve_types::object_ref,
		curve_types::unique, 
		curve_types::vector_int,
		curve_types::vector_float,
		curve_types::vector_object_ref,
		curve_types::vector_unique>;

	struct curve_t {};

	struct curve : public resource_type<curve_t>
	{
		curve_type curve_type = curve_type::error;
		curve_variable_type data_type = curve_variable_type::error;
		bool sync = false,
			save = false,
			trim = false;
		curve_default_value default_value{};
	};

	[[nodiscard]] curve_default_value reset_default_value(const curve &c);

	constexpr bool is_set(const curve_default_value&) noexcept;
	bool is_curve_valid(const resources::curve&) noexcept;
	bool is_curve_valid(const resources::curve&, const curve_default_value&) noexcept;

	curve_default_value curve_from_string(const resources::curve &c, std::string_view str);
	curve_default_value curve_from_node(const resources::curve&, const data::parser_node&);
}

namespace hades
{
	string to_string(const resources::curve &c);
	string to_string(std::tuple<const resources::curve&, const resources::curve_default_value&> curve);

	string curve_to_string(const resources::curve &c, const resources::curve_default_value &v);
}

#endif // !HADES_CURVE_EXTRA_HPP
