#include "hades/curve_extra.hpp"

#include <string_view>

#include "hades/parser.hpp"

namespace hades
{
	template<>
	std::monostate from_string<std::monostate>(std::string_view)
	{
		LOGWARNING("Tried to call from_string<std::monostate>, a curve is being set without being properly initialised");
		return {};
	}
}

namespace hades::resources
{
	curve_type read_curve_type(std::string_view s) noexcept
	{
		using namespace std::string_view_literals;
		if (s == "const"sv)
			return curve_type::const_c;
		else if (s == "step"sv)
			return curve_type::step;
		else if (s == "linear"sv)
			return curve_type::linear;
		else if (s == "pulse"sv)
			return curve_type::pulse;
		else
			return curve_type::error;
	}

	curve_variable_type read_variable_type(std::string_view s) noexcept
	{
		using namespace std::string_view_literals;
		if (s == "int"sv)
			return curve_variable_type::int_t;
		else if (s == "float"sv)
			return curve_variable_type::float_t;
		else if (s == "bool"sv)
			return curve_variable_type::bool_t;
		else if (s == "string"sv)
			return curve_variable_type::string;
		else if (s == "obj_ref"sv)
			return curve_variable_type::object_ref;
		else if (s == "unique"sv)
			return curve_variable_type::unique;
		else if (s == "int_vector"sv)
			return curve_variable_type::vector_int;
		else if (s == "float_vector"sv)
			return curve_variable_type::vector_float;
		else if (s == "obj_ref_vector"sv)
			return curve_variable_type::vector_object_ref;
		else if (s == "unique_vector"sv)
			return curve_variable_type::vector_unique;
		else
			return curve_variable_type::error;
	}

	[[nodiscard]] curve_default_value reset_default_value(const curve& c)
	{
		using resources::curve_variable_type;
		if (c.data_type == curve_variable_type::error)
			throw invalid_curve{"Tried to call reset_default_value on an invalid curve"};

		using namespace resources::curve_types;

		curve_default_value default_value{};

		switch (c.data_type)
		{
		case curve_variable_type::int_t:
			default_value.emplace<int_t>();
			break;
		case curve_variable_type::float_t:
			default_value.emplace<float_t>();
			break;
		case curve_variable_type::bool_t:
			default_value.emplace<bool_t>();
			break;
		case curve_variable_type::string:
			default_value.emplace<resources::curve_types::string>();
			break;
		case curve_variable_type::object_ref:
			default_value.emplace<object_ref>();
			break;
		case curve_variable_type::unique:
			default_value.emplace<unique>();
			break;
		case curve_variable_type::vector_int:
			default_value.emplace<vector_int>();
			break;
		case curve_variable_type::vector_float:
			default_value.emplace<vector_float>();
			break;
		case curve_variable_type::vector_object_ref:
			default_value.emplace<vector_object_ref>();
			break;
		case curve_variable_type::vector_unique:
			default_value.emplace<vector_unique>();
			break;
		}

		return default_value;
	}

	curve_default_value get_default_value(const data::parser_node &n,
		const curve &current_value, hades::unique_id mod)
	{
		using namespace std::string_view_literals;
		constexpr auto property_name = "default_value"sv;

		if (!is_curve_valid(current_value))
			throw invalid_curve{ "Tried to call get_default_value on an invalid curve" };

		const auto value_node = n.get_child(property_name);

		if (!value_node)
			return current_value.default_value;

		return curve_from_node(current_value, *value_node);
	}

	void parse_curves(unique_id mod, const data::parser_node &n, data::data_manager &d)
	{
		//curves:
		//		name:
		//			type: default: step //determines how the values are read between keyframes
		//			value: default: int32 //determines the value type
		//			sync: default: false //true if this should be syncronised to the client
		//			save: default false //true if this should be saved when creating a save file
		//			default: value, [value1, value2, value3, ...] etc

		//these are loaded into the game instance before anything else
		//curves cannot change type/value once they have been created,
		//the resource is used to created synced curves of the correct type
		//on clients
		//a blast of names to VariableIds must be sent on client connection
		//so that we can refer to curves by id instead of by string

		//first check that our node is valid
		//no point looping though if their are not children
		using namespace std::string_view_literals;
		constexpr auto resource_type = "curve"sv;
		const auto curves = n.get_children();

		for (const auto &c : curves)
		{
			const auto name = c->to_string();
			const auto id = d.get_uid(name);

			auto new_curve = d.find_or_create<curve>(id, mod);

			const auto old_type = new_curve->data_type;

			using namespace data::parse_tools;
			new_curve->curve_type	= get_scalar(*c, "type"sv,	new_curve->curve_type, read_curve_type);
			new_curve->data_type	= get_scalar(*c, "value"sv, new_curve->data_type, read_variable_type);
			new_curve->sync			= get_scalar(*c, "sync"sv,  new_curve->sync);
			new_curve->save			= get_scalar(*c, "save"sv,  new_curve->save);

			if (old_type != new_curve->data_type)
				new_curve->default_value = reset_default_value(*new_curve);

			new_curve->default_value = get_default_value(*c, *new_curve, mod);

			if (!is_curve_valid(*new_curve))
				throw invalid_curve{ "get_default_value returned an invalid curve" };
		}
	}

	constexpr bool is_set(const curve_default_value &v) noexcept
	{
		return !std::holds_alternative<std::monostate>(v);
	}

	bool is_curve_valid(const resources::curve &c) noexcept
	{
		return is_curve_valid(c, c.default_value);
	}

	bool is_curve_valid(const resources::curve &c, const curve_default_value &v) noexcept
	{
		if (c.curve_type == curve_type::error)
			return false;

		using resources::curve_variable_type;
		if (c.data_type == curve_variable_type::error)
			return false;

		using namespace resources::curve_types;
		switch (c.data_type)
		{
		case curve_variable_type::int_t:
			return std::holds_alternative<int_t>(v);
		case curve_variable_type::float_t:
			return std::holds_alternative<float_t>(v);
		case curve_variable_type::bool_t:
			return std::holds_alternative<bool_t>(v);
		case curve_variable_type::string:
			return std::holds_alternative<resources::curve_types::string>(v);
		case curve_variable_type::object_ref:
			return std::holds_alternative<object_ref>(v);
		case curve_variable_type::unique:
			return std::holds_alternative<unique>(v);
		case curve_variable_type::vector_int:
			return std::holds_alternative<vector_int>(v);
		case curve_variable_type::vector_float:
			return std::holds_alternative<vector_float>(v);
		case curve_variable_type::vector_object_ref:
			return std::holds_alternative<vector_object_ref>(v);
		case curve_variable_type::vector_unique:
			return std::holds_alternative<vector_unique>(v);
		default:
			return false;
		}
	}



	resources::curve_default_value curve_from_string(const resources::curve &c, std::string_view str)
	{
		if (!resources::is_curve_valid(c))
			throw invalid_curve{ "Tried to call curve_from_string to set an invalid curve" };

		auto out = c.default_value;

		std::visit([str](auto &&v) {
			using T = std::decay_t<decltype(v)>;

			using namespace resources::curve_types;
			if constexpr (is_vector_type_v<T>)
				v = vector_from_string<T>(str);
			else
				v = from_string<T>(str);
		}, out);

		return out;
	}

	curve_default_value curve_from_node(const resources::curve &c, const data::parser_node &n)
	{
		auto value = reset_default_value(c);

		std::visit([&n](auto &&v) {
			using T = std::decay_t<decltype(v)>;

			if constexpr (curve_types::is_vector_type_v<T>)
				v = n.to_sequence<T::value_type>();
			else
				v = n.to_scalar<T>();
		}, value);

		return value;
	}
}

namespace hades
{
	template<resources::curve_variable_type t>
	types::string to_string(const resources::curve_default_value &v)
	{
		using T = resources::as_curve_type_t<t>;
		const auto value = resources::get_curve_default_value<T>(v);
		return to_string(value);
	}

	template<>
	types::string to_string<std::monostate>(std::monostate value)
	{
		LOGWARNING("Tried to call to_string<std::monostate>, a curve is being written without being properly initialised");
		return {};
	}

	types::string to_string(const resources::curve &v)
	{
		return curve_to_string(v, v.default_value);
	}

	string to_string(std::tuple<const resources::curve&, const resources::curve_default_value&> curve)
	{
		return curve_to_string(std::get<0>(curve), std::get<1>(curve));
	}

	types::string curve_to_string(const resources::curve &c, const resources::curve_default_value &v)
	{
		if (!resources::is_curve_valid(c) ||
			!resources::is_set(v))
			throw invalid_curve{ "Tried to call to_string on an invalid curve" };

		return std::visit([](auto&& t)->types::string {
			using T = std::decay_t<decltype(t)>;
			if constexpr (resources::curve_types::is_vector_type_v<T>)
				return to_string(std::begin(t), std::end(t));
			else
				return to_string(t);
		}, v);
	}

	void register_curve_resource(data::data_manager &d)
	{
		d.register_resource_type("curves", resources::parse_curves);
	}
}