#ifndef HADES_CURVE_EXTRA_HPP
#define HADES_CURVE_EXTRA_HPP

#include <type_traits>
#include <variant>
#include <vector>

#include "hades/curve.hpp"
#include "hades/data.hpp"
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
	enum VariableType { ERROR, INT, FLOAT, BOOL, STRING, OBJECT_REF, UNIQUE, VECTOR_INT, VECTOR_FLOAT, VECTOR_OBJECT_REF, VECTOR_UNIQUE };

	namespace curve_types
	{
		using int_t = hades::types::int32;
		using float_t = float;
		using bool_t = bool;
		using string = hades::types::string;
		//TODO: might be dangerous to have this alias to the same type as int_t
		using object_ref = int_t; //TODO: entity ID type hades::EntityId
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

		//NOTE: if object_ref is ever given a strong typedef, 
		// or has a different base variable, then this specialisation should be re-enabled
		//template <>
		//struct is_vector_type<vector_object_ref> : public std::true_type {};

		template <>
		struct is_vector_type<vector_unique> : public std::true_type {};

		template<typename T>
		constexpr auto is_vector_type_v = is_vector_type<T>::value;
	}

	using curve_default_value = std::variant<
		curve_types::int_t, 
		curve_types::float_t,
		curve_types::bool_t, 
		curve_types::string, 
		curve_types::unique, 
		curve_types::vector_int,
		curve_types::vector_float,
		curve_types::vector_unique>;

	struct curve_t {};

	struct curve : public resource_type<curve_t>
	{
		curve_type curve_type = curve_type::error;
		VariableType data_type = VariableType::ERROR;
		bool sync = false,
			save = false,
			trim = false;
		curve_default_value default_value;
	};

	bool is_curve_valid(const resources::curve&) noexcept;
}

#endif // !HADES_CURVE_EXTRA_HPP
