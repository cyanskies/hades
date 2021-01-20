#include "hades/parser.hpp"

#include "hades/data.hpp"
#include "hades/logging.hpp"

namespace hades::data
{
	namespace detail
	{
		template<typename T, typename Converter>
		T convert(std::string_view str, Converter &&conv)
		{
			if constexpr (std::is_invocable_r_v<T, Converter, std::string_view>)
				return std::invoke(std::forward<Converter>(conv), str);
			else
			{
				std::ignore = conv;
				return from_string<T>(str);
			}
		}
	}

	template<typename T, typename Converter>
	T parser_node::to_scalar(Converter&& conv) const
	{
		return detail::convert<T>(to_string(), std::forward<Converter>(conv));
	}

	template<typename T, typename Converter>
	inline std::vector<T> parser_node::merge_sequence(std::vector<T> current, Converter conv) const
	{
		enum class sequence_mode {
			add,
			replace,
			remove
		};

		const auto children = get_children();
		std::vector<T> result{ std::move(current) };
		result.reserve(result.size() + children.size());
		auto mode = sequence_mode::add;
		for (const auto &child : children)
		{
			using namespace std::string_view_literals;
			constexpr auto replace = "="sv;
			constexpr auto add = "+"sv;
			constexpr auto remove = "-"sv;

			const auto value = child->to_string();

			if (value == add)
				mode = sequence_mode::add;
			else if (value == remove)
				mode = sequence_mode::remove;
			else if (value == replace)
			{
				result.clear();
				result.reserve(children.size());
				continue;
			}

			if(mode == sequence_mode::add)
				result.emplace_back(detail::convert<T>(child->to_string(), conv));
			else if (mode == sequence_mode::remove)
			{
				//remove anything equal to the value v
				const auto v = detail::convert<T>(child->to_string(), conv);
				result.erase(std::remove_if(std::begin(result), std::end(result), [&v](auto &&other) {
					return v == other;
				}), std::end(result));
			}
		}

		return result;
	}

	template<typename T>
	inline std::vector<T> parser_node::merge_sequence(std::vector<T> current) const
	{
		return merge_sequence(std::move(current), nullptr);
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
		inline T get_scalar(const parser_node &node, std::string_view property_name, T default_value,
			ConversionFunc convert)
		{
			// NOTE: default to_string<unique_id> calls get_uid() const, which
			// wont define a id if it didn't already exist, this always leads
			// to bugs in parsing code
			static_assert(!(std::is_same_v<T, unique_id> && std::is_same_v<ConversionFunc, nullptr_t>),
				"call get_unique instead");

			const auto property_node = node.get_child(property_name);
			if (property_node)
				return property_node->to_scalar<T>(convert);
			else
				return default_value;
		}

		template<class T, typename ConversionFunc>
		inline std::vector<T> get_sequence(const parser_node &node, std::string_view property_name, 
			const std::vector<T> &default_value, ConversionFunc convert)
		{
			//NOTE: see get_scalar above
			static_assert(!(std::is_same_v<T, unique_id> && std::is_same_v<ConversionFunc, nullptr_t>),
				"call get_unique_sequence instead");

			const auto property_node = node.get_child(property_name);
			if (property_node)
				return property_node->to_sequence<T>(convert);
			else
				return default_value;
		}

		template<class T, typename ConversionFunc>
		inline std::vector<T> merge_sequence(const parser_node & node,
			std::string_view property_name, std::vector<T> current_value, ConversionFunc convert)
		{
			const auto property_node = node.get_child(property_name);
			if (property_node)
				return property_node->merge_sequence<T>(std::move(current_value), convert);
			else
				return current_value;
		}

		inline unique_id get_unique(const parser_node& node,
			std::string_view property_name, unique_id default_value)
		{
			return get_scalar(node, property_name, default_value, &make_uid);
		}

		inline std::vector<unique_id> get_unique_sequence(const parser_node& node,
			std::string_view property_name, const std::vector<unique_id>& default_value)
		{
			return get_sequence(node, property_name, default_value, &make_uid);
		}

		inline std::vector<unique_id> merge_unique_sequence(const parser_node& node,
			std::string_view property_name, std::vector<unique_id> current_value)
		{
			return merge_sequence(node, property_name, std::move(current_value), &make_uid);
		}
	}
}