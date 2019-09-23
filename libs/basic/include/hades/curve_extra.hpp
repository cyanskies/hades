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
	enum class curve_variable_type { error, int_t, float_t, vec2_float, bool_t,
		string, object_ref, unique, collection_int, collection_float,
		collection_object_ref, collection_unique };

	namespace curve_types
	{
		using int_t = int32;
		using float_t = float32;
		using vec2_float = vector_float;
		using bool_t = bool;
		using string = types::string;
		using object_ref = entity_id;
		using unique = unique_id;
		using collection_int = std::vector<int_t>;
		using collection_float = std::vector<float_t>;
		using collection_object_ref = std::vector<object_ref>;
		using collection_unique = std::vector<unique>;

		template <typename T>
		struct is_vector_type : std::false_type {};
		template <>
		struct is_vector_type<vec2_float> : public std::true_type {};

		template <typename T>
		constexpr auto is_vector_type_v = is_vector_type<T>::value;

		template <typename T>
		struct is_collection_type : public std::false_type {};
		template <>
		struct is_collection_type<collection_int> : public std::true_type {};
		template <>
		struct is_collection_type<collection_float> : public std::true_type {};
		template <>
		struct is_collection_type<collection_object_ref> : public std::true_type {};
		template <>
		struct is_collection_type<collection_unique> : public std::true_type {};

		template<typename T>
		constexpr auto is_collection_type_v = is_collection_type<T>::value;

		template<typename T>
		struct is_curve_type : public std::false_type {};

		template<>
		struct is_curve_type<int_t> : public std::true_type {};
		template<>
		struct is_curve_type<float_t> : public std::true_type {};
		template<>
		struct is_curve_type<vec2_float> : public std::true_type {};
		template<>
		struct is_curve_type<bool_t> : public std::true_type {};
		template<>
		struct is_curve_type<string> : public std::true_type {};
		template<>
		struct is_curve_type<object_ref> : public std::true_type {};
		template<>
		struct is_curve_type<unique> : public std::true_type {};
		template<>
		struct is_curve_type<collection_int> : public std::true_type {};
		template<>
		struct is_curve_type<collection_float> : public std::true_type {};
		template<>
		struct is_curve_type<collection_object_ref> : public std::true_type {};
		template<>
		struct is_curve_type<collection_unique> : public std::true_type {};

		template<typename T>
		constexpr auto is_curve_type_v = is_curve_type<T>::value;
	}

	using curve_default_value = std::variant<
		std::monostate,
		curve_types::int_t, 
		curve_types::float_t,
		curve_types::vec2_float,
		curve_types::bool_t, 
		curve_types::string, 
		curve_types::object_ref,
		curve_types::unique, 
		curve_types::collection_int,
		curve_types::collection_float,
		curve_types::collection_object_ref,
		curve_types::collection_unique>;

	struct curve_t {};

	struct curve : public resource_type<curve_t>
	{
        curve_type c_type = curve_type::error;
		curve_variable_type data_type = curve_variable_type::error;
		bool sync = false, //sync shares the value with clients
			save = false, //save records the value in save files
			trim = false; //[depricated(should just be !save)]old data in the curve can be removed to save space
		curve_default_value default_value{};
	};

	[[nodiscard]] curve_default_value reset_default_value(const curve &c);

	constexpr bool is_set(const curve_default_value& v) noexcept
	{
		return !std::holds_alternative<std::monostate>(v);
	}
	
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
