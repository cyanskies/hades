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
				return from_string<T>(str);
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
			result.emplace_back(detail::convert<T>(child->to_string(), conv));

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
		template<class T, typename ConversionFunc>
		T get_scalar(const parser_node &node, std::string_view property_name, T default_value,
			ConversionFunc convert)
		{
			const auto property_node = node.get_child(property_name);
			if (property_node)
				return property_node->to_scalar<T>(convert);
			else
				return default_value;
		}

		template<class T, typename ConversionFunc>
		std::vector<T> get_sequence(const parser_node &node, std::string_view property_name, 
			const std::vector<T> &default_value, ConversionFunc convert)
		{
			const auto property_node = node.get_child(property_name);
			if (property_node)
				return property_node->to_sequence<T>(convert);
			else
				return default_value;
		}
	}
}