#include "hades/curve_extra.hpp"

#include <string_view>

#include "hades/parser.hpp"

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

	VariableType read_variable_type(std::string_view s) noexcept
	{
		using namespace std::string_view_literals;
		if (s == "int"sv)
			return VariableType::INT;
		else if (s == "float"sv)
			return VariableType::FLOAT;
		else if (s == "bool"sv)
			return VariableType::BOOL;
		else if (s == "string"sv)
			return VariableType::STRING;
		else if (s == "obj_ref"sv)
			return VariableType::OBJECT_REF;
		else if (s == "unique"sv)
			return VariableType::UNIQUE;
		else if (s == "int_vector"sv)
			return VariableType::VECTOR_INT;
		else if (s == "float_vector"sv)
			return VariableType::VECTOR_FLOAT;
		else if (s == "obj_ref_vector"sv)
			return VariableType::VECTOR_OBJECT_REF;
		else if (s == "unique_vector"sv)
			return VariableType::VECTOR_UNIQUE;
		else
			return VariableType::ERROR;
	}

	void reset_default_value(curve& c)
	{
		using resources::VariableType;
		if (c.data_type == VariableType::ERROR)
			throw invalid_curve{"Tried to call reset_default_value on an invalid curve"};

		using namespace resources::curve_types;

		switch (c.data_type)
		{
		case VariableType::INT:
			c.default_value.emplace<int_t>();
			return;
		case VariableType::FLOAT:
			c.default_value.emplace<float_t>();
			return;
		case VariableType::BOOL:
			c.default_value.emplace<bool_t>();
			return;
		case VariableType::STRING:
			c.default_value.emplace< resources::curve_types::string>();
			return;
		case VariableType::OBJECT_REF:
			c.default_value.emplace<object_ref>();
			return;
		case VariableType::UNIQUE:
			c.default_value.emplace<unique>();
			return;
		case VariableType::VECTOR_INT:
			c.default_value.emplace<vector_int>();
			return;
		case VariableType::VECTOR_FLOAT:
			c.default_value.emplace<vector_float>();
			return;
		case VariableType::VECTOR_OBJECT_REF:
			c.default_value.emplace<vector_object_ref>();
			return;
		case VariableType::VECTOR_UNIQUE:
			c.default_value.emplace<vector_unique>();
			return;
		}
	}
	curve_default_value get_default_value(const data::parser_node &n, std::string_view resource_name,
		const curve &current_value, hades::unique_id mod)
	{
		using namespace std::string_view_literals;
		constexpr auto resource_type = "curve"sv;
		constexpr auto property_name = "default_value"sv;

		if (!is_curve_valid(current_value))
			throw invalid_curve{ "Tried to call get_default_value on an invalid curve" };

		const auto value_node = n.get_child(property_name);

		if (!value_node)
			return current_value.default_value;

		auto out = current_value.default_value;

		std::visit([&current_value, &value_node](auto &&v) {
			using T = std::decay_t<decltype(v)>;

			if constexpr (curve_types::is_vector_type_v<T>)
				v = value_node->to_sequence<T::value_type>();
			else
				v = value_node->to_scalar<T>();
		}, out);

		return out;
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
			new_curve->curve_type	= get_scalar(*c, resource_type, name, "type"sv,	 new_curve->curve_type, mod, read_curve_type);
			new_curve->data_type	= get_scalar(*c, resource_type, name, "value"sv, new_curve->data_type, mod, read_variable_type);
			new_curve->sync			= get_scalar(*c, resource_type, name, "sync"sv,  new_curve->sync, mod);
			new_curve->save			= get_scalar(*c, resource_type, name, "save"sv,  new_curve->save, mod);

			if (old_type != new_curve->data_type)
				reset_default_value(*new_curve);

			new_curve->default_value = get_default_value(*c, name, *new_curve, mod);

			if (!is_curve_valid(*new_curve))
				throw invalid_curve{ "get_default_value returned an invalid curve" };
		}
	}

	bool is_curve_valid(const resources::curve &c) noexcept
	{
		using resources::VariableType;
		if (c.data_type == VariableType::ERROR)
			return false;

		using namespace resources::curve_types;

		switch (c.data_type)
		{
		case VariableType::INT:
			return std::holds_alternative<int_t>(c.default_value);
		case VariableType::FLOAT:
			return std::holds_alternative<float_t>(c.default_value);
		case VariableType::BOOL:
			return std::holds_alternative<bool_t>(c.default_value);
		case VariableType::STRING:
			return std::holds_alternative<resources::curve_types::string>(c.default_value);
		case VariableType::OBJECT_REF:
			return std::holds_alternative<object_ref>(c.default_value);
		case VariableType::UNIQUE:
			return std::holds_alternative<unique>(c.default_value);
		case VariableType::VECTOR_INT:
			return std::holds_alternative<vector_int>(c.default_value);
		case VariableType::VECTOR_FLOAT:
			return std::holds_alternative<vector_float>(c.default_value);
		case VariableType::VECTOR_OBJECT_REF:
			return std::holds_alternative<vector_object_ref>(c.default_value);
		case VariableType::VECTOR_UNIQUE:
			return std::holds_alternative<vector_unique>(c.default_value);
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
}

namespace hades
{
	template<resources::VariableType t>
	types::string to_string(const resources::curve_default_value &v)
	{
		using T = resources::as_curve_type_t<t>;
		const auto value = resources::get_curve_default_value<T>(v);
		return to_string(value);
	}

	types::string to_string(const resources::curve &v)
	{
		if (v.data_type == resources::VariableType::ERROR 
			|| !resources::is_curve_valid(v))
			throw invalid_curve{"Tried to call to_string on an invalid curve"};

		return std::visit([](auto&& t)->types::string
		{
			using T = std::decay_t<decltype(t)>;
			if constexpr(resources::curve_types::is_vector_type_v<T>)
				return to_string(std::begin(t), std::end(t));
			else
				return to_string<T>(t);
		}, v.default_value);
	}

	void register_curve_resource(data::data_manager &d)
	{
		d.register_resource_type("curves", resources::parse_curves);
	}
}