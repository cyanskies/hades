#include "hades/parser.hpp"

#include "hades/data.hpp"
#include "hades/logging.hpp"

namespace hades::data
{
	namespace detail
	{
		template<typename T, typename Converter>
		T convert(std::string_view str, Converter conv)
		{
			if constexpr (std::is_invocable_r_v<T, Converter, std::string_view>)
				return std::invoke(conv, str);
			else
				return stov<T>(to_string(str)); //TODO: from_string
		}
	}

	template<typename T, typename Converter>
	T parser_node::to_scalar(Converter conv) const
	{
		return detail::convert<T>(to_string(), conv);
	}

	template<typename T, typename Converter>
	std::vector<T> parser_node::to_sequence(Converter conv) const
	{
		const auto children = get_children();
		std::vector<T> result;
		result.reserve(children.size());
		for (const auto &child : children)
			result.emplace_back(detail::convert(child->to_string(), conv));

		return result;
	}

	template<typename Value, typename Converter>
	std::vector<std::pair<string, Value>> parser_node::to_map(Converter conv) const
	{
		std::vector<std::pair<string, Value>> result;

		const auto seq = get_children();

		for (const auto &n : seq)
		{
			const auto value = n->get_child();
			result.emplace_back(std::make_pair(n->to_string(), value->to_scalar<Value>(conv)));
		}

		return result;
	}

	template<typename Key, typename Value, typename KeyConv, typename ValueConv>
	std::vector<std::pair<Key, Value>> parser_node::to_map(KeyConv k_conv, ValueConv v_conv) const
	{
		std::vector<std::pair<Key, Value>> result;

		const auto seq = get_children();

		for (const auto &n : seq)
		{
			const auto value = n->get_child();
			result.emplace_back(std::make_pair(n->to_scalar<Key>(k_conv), value->to_scalar<Value>(v_conv)));
		}

		return result;
	}

	namespace parse_tools
	{
		inline void log_parse_error(std::string_view resource_type, std::string_view resource_name,
			std::string_view property_name, std::string_view requested_type, hades::unique_id mod)
		{
			using namespace std::string_literals;
			auto message = "Error parsing data"s;
			if (mod != hades::unique_id::zero)
			{
				const auto mod_ptr = get<hades::resources::mod>(mod);
				message += ", in mod: "s + mod_ptr->name;
			}
			message += ", type: "s + hades::to_string(resource_type)
				+ ", name: "s + hades::to_string(resource_name)
				+ ", for property: "s + hades::to_string(property_name)
				+ ". value must be "s + hades::to_string(requested_type);
			LOGERROR(message);
		}

		template<class T, typename ConversionFunc>
		T get_scalar(const parser_node &node, std::string_view resource_type, std::string_view resource_name,
			std::string_view property_name, T default_value, hades::unique_id mod, ConversionFunc convert)
		{
			const auto property_node = node.get_child(property_name);
			if (property_node)
				return property_node->to_scalar<T>(convert);
			else
				return default_value;
		}

		template<typename ConversionFunc>
		unique_id get_scalar(const parser_node &node, std::string_view resource_type, std::string_view resource_name,
			std::string_view property_name, unique_id default_value, hades::unique_id mod, ConversionFunc convert)
		{
			return get_scalar(node, resource_type, resource_name, property_name, mode, default_value, [](std::string_view str) {
				return make_uid(str);
			})
		}

		template<class T, typename ConversionFunc>
		T get_scalar(const parser_node &node, std::string_view resource_type, std::string_view resource_name,
			std::string_view property_name, T default_value, ConversionFunc convert)
		{
			return get_scalar<T>(node, resource_type, resource_name, property_name, default_value, hades::unique_id::zero, convert);
		}
	}
}