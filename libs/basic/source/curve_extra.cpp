#include "hades/curve_extra.hpp"

#include <string_view>
#include <type_traits>

#include "hades/parser.hpp"

namespace hades
{
	template<>
	std::monostate from_string<std::monostate>(std::string_view)
	{
		LOGWARNING("Tried to call from_string<std::monostate>, a curve is being set without being properly initialised");
		return {};
	}

	template<>
	curve_types::time_d from_string<curve_types::time_d>(std::string_view s) noexcept
	{
		return duration_from_string(s);
	}
}

namespace hades::resources
{
	//curve_variable_type_from_string
	curve_variable_type read_variable_type(std::string_view s) noexcept
	{
		using namespace std::string_view_literals;
		if (s == "int32"sv)
			return curve_variable_type::int_t;
		/*else if (s == "int64"sv)
			return curve_variable_type::int64_t;*/
		else if (s == "float"sv)
			return curve_variable_type::float_t;
		else if (s == "vec2-float"sv)
			return curve_variable_type::vec2_float;
		else if (s == "bool"sv)
			return curve_variable_type::bool_t;
		else if (s == "string"sv)
			return curve_variable_type::string;
		else if (s == "obj-ref"sv)
			return curve_variable_type::object_ref;
		else if (s == "unique"sv)
			return curve_variable_type::unique;
		else if (s == "colour"sv)
			return curve_variable_type::colour;
		else if (s == "time-d"sv)
			return curve_variable_type::time_d;
		else if (s == "collection-int32"sv)
			return curve_variable_type::collection_int;
		else if (s == "collection-float"sv)
			return curve_variable_type::collection_float;
		else if (s == "collection-obj-ref"sv)
			return curve_variable_type::collection_object_ref;
		else if (s == "collection-unique"sv)
			return curve_variable_type::collection_unique;
		else if (s == "collection-colour"sv)
			return curve_variable_type::collection_colour;
		else if (s == "collection-time"sv)
			return curve_variable_type::collection_time_d;
		else
			return curve_variable_type::error;
	}

	[[nodiscard]] curve_default_value reset_default_value(const curve& c)
	{
		using resources::curve_variable_type;
		using namespace resources::curve_types;

		curve_default_value default_value{};

		switch (c.data_type)
		{
		case curve_variable_type::int_t:
			default_value.emplace<int_t>();
			break;
		/*case curve_variable_type::int64_t:
			default_value.emplace<int64_t>();
			break;*/
		case curve_variable_type::float_t:
			default_value.emplace<float_t>();
			break;
		case curve_variable_type::vec2_float:
			default_value.emplace<vec2_float>();
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
			default_value.emplace<unique>(unique_zero);
			break;
		case curve_variable_type::colour:
			// TODO: pick a good default colour?(default constructor will pick black)
			default_value.emplace<colour>();
			break;
		case curve_variable_type::time_d:
			default_value.emplace<time_d>();
			break;
		case curve_variable_type::collection_int:
			default_value.emplace<collection_int>();
			break;
		case curve_variable_type::collection_float:
			default_value.emplace<collection_float>();
			break;
		case curve_variable_type::collection_object_ref:
			default_value.emplace<collection_object_ref>();
			break;
		case curve_variable_type::collection_unique:
			default_value.emplace<collection_unique>();
			break;
		case curve_variable_type::collection_colour:
			default_value.emplace<collection_colour>();
			break;
		case curve_variable_type::collection_time_d:
			default_value.emplace<collection_time_d>();
			break;
		case curve_variable_type::error:
			[[fallthrough]];
		default:
			throw invalid_curve{ to_string(c.id) + " is an invalid curve type, it may not have been registered" };
		}

		return default_value;
	}

	curve_default_value get_default_value(const data::parser_node &n,
		const curve &current_value, hades::unique_id mod)
	{
		using namespace std::string_view_literals;
		constexpr auto property_name = "default_value"sv;

		if (!is_curve_valid(current_value))
			throw invalid_curve{ "Tried to call get_default_value on an invalid curve while parsing mod: " + to_string(mod) };

		const auto value_node = n.get_child(property_name);

		if (!value_node)
			return current_value.default_value;

		return curve_from_node(current_value, *value_node);
	}

	void parse_curves(unique_id mod, const data::parser_node& n, data::data_manager& d)
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

		for (const auto& c : curves)
		{
			const auto name = c->to_string();
			const auto id = d.get_uid(name);

			auto new_curve = d.find_or_create<curve>(id, mod);

			const auto old_type = new_curve->data_type;

			using namespace data::parse_tools;
			new_curve->data_type = get_scalar(*c, "value"sv, new_curve->data_type, read_variable_type);
			new_curve->sync = get_scalar(*c, "sync"sv, new_curve->sync);
			new_curve->save = get_scalar(*c, "save"sv, new_curve->save);

			if (old_type != new_curve->data_type)
				new_curve->default_value = reset_default_value(*new_curve);

			new_curve->default_value = get_default_value(*c, *new_curve, mod);

			if (!is_curve_valid(*new_curve))
				throw invalid_curve{ "get_default_value returned an invalid curve" };

			detail::add_to_curve_master_list(new_curve);
		}
	}

	bool is_curve_valid(const resources::curve &c) noexcept
	{
		return is_curve_valid(c, c.default_value);
	}

	bool is_curve_valid(const resources::curve &c, const curve_default_value &v) noexcept
	{
		using resources::curve_variable_type;
		if (c.data_type == curve_variable_type::error)
			return false;

		using namespace resources::curve_types;
		switch (c.data_type)
		{
		case curve_variable_type::int_t:
			return std::holds_alternative<int_t>(v);
		/*case curve_variable_type::int64_t:
			return std::holds_alternative<int64_t>(v);*/
		case curve_variable_type::float_t:
			return std::holds_alternative<float_t>(v);
		case curve_variable_type::vec2_float:
			return std::holds_alternative<vec2_float>(v);
		case curve_variable_type::bool_t:
			return std::holds_alternative<bool_t>(v);
		case curve_variable_type::string:
			return std::holds_alternative<resources::curve_types::string>(v);
		case curve_variable_type::object_ref:
			return std::holds_alternative<object_ref>(v);
		case curve_variable_type::unique:
			return std::holds_alternative<unique>(v);
		case curve_variable_type::colour:
			return std::holds_alternative<colour>(v);
		case curve_variable_type::time_d:
			return std::holds_alternative<time_d>(v);
		case curve_variable_type::collection_int:
			return std::holds_alternative<collection_int>(v);
		case curve_variable_type::collection_float:
			return std::holds_alternative<collection_float>(v);
		case curve_variable_type::collection_object_ref:
			return std::holds_alternative<collection_object_ref>(v);
		case curve_variable_type::collection_unique:
			return std::holds_alternative<collection_unique>(v);
		case curve_variable_type::collection_colour:
			return std::holds_alternative<collection_colour>(v);
		case curve_variable_type::collection_time_d:
			return std::holds_alternative<collection_time_d>(v);
		default:
			return false;
		}
	}

	template<typename T>
	static T vector_t_from_string(std::string_view str)
	{
		static_assert(resources::curve_types::is_vector_type_v<T>);
		const auto elms = vector_from_string<std::vector<typename T::value_type>>(str);
		const auto siz = std::size(elms);

		if (siz != 2u)
		{
			if(siz == 0u)
			{
				//TODO: warn, bad
				using Ty = typename T::value_type;
				return { Ty{}, Ty{} };
			}

			//size must be > 2
			//TODO: warn excess elms lost
		}

		auto out = T{};
		
		out.x = elms[0];
		if (siz != 1)
			out.y = elms[1];

		return out;
	}

	curve_default_value curve_from_node(const resources::curve &c, const data::parser_node &n)
	{
		auto value = reset_default_value(c);

		std::visit([&n](auto &&v) {
			using T = std::decay_t<decltype(v)>;

			if constexpr (curve_types::is_vector_type_v<T>)
			{
				using value_type = typename T::value_type;
				const auto seq = n.to_sequence<value_type>();
				const auto size = std::size(seq);
				if (size < 2)
				{
					//TODO: warn bad, leave default state
					return;
				}
				else if (size > 2)
				{
					//TODO: warn, excess elements ignored
				}

				v = T{ seq[0], seq[1] };
			}
			else if constexpr (curve_types::is_collection_type_v<T>)
			{
                using ValueType = typename T::value_type;
                if constexpr (std::is_same_v<ValueType, unique_id>)
                    v = n.to_sequence<ValueType>(data::make_uid);
				else
                    v = n.to_sequence<ValueType>();
			}
			else
			{
				const auto value = n.get_child();
				if constexpr (std::is_same_v<T, unique_id>)
					v = value->to_scalar<T>(data::make_uid);
				else
					v = value->to_scalar<T>();
			}
		}, value);

		return value;
	}

	static std::vector<const curve*> curve_master_list;

	const std::vector<const curve*> &get_all_curves()
	{
		return curve_master_list;
	}

	void detail::add_to_curve_master_list(const curve* c)
	{
		curve_master_list.emplace_back(c);
		remove_duplicates(curve_master_list);
		return;
	}
}

namespace hades
{
	static types::string to_string(std::monostate value) noexcept
	{
		std::ignore = value;
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

	template<typename T>
	static string vector_t_to_string(const T &v)
	{
		static_assert(resources::curve_types::is_vector_type_v<T>);
		return "[" + to_string(v.x) + ", " + to_string(v.y) + "]";
	}

	types::string curve_to_string(const resources::curve &c, const resources::curve_default_value &v)
	{
		if (!resources::is_curve_valid(c) ||
			!resources::is_set(v))
			throw invalid_curve{ "Tried to call to_string on an invalid curve" };

		return std::visit([](auto&& t)->types::string {
			using T = std::decay_t<decltype(t)>;
			if constexpr (resources::curve_types::is_collection_type_v<T>)
				return to_string(std::begin(t), std::end(t));
			else if constexpr (resources::curve_types::is_vector_type_v<T>)
				return vector_t_to_string(t);
			else
				return to_string(t);
		}, v);
	}

	void register_curve_resource(data::data_manager &d)
	{
		d.register_resource_type("curves", resources::parse_curves);
	}
}
