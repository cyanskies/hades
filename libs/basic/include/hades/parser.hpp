#ifndef HADES_PARSER_HPP
#define HADES_PARSER_HPP

#include <memory>
#include <vector>

#include "hades/types.hpp"
#include "hades/uniqueid.hpp"

namespace hades::data
{
	//thrown by all the to_* functions in the parser_node class
	class parser_convert_exception : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};
	//parser_node, represents either string node

	class parser_node
	{
	public:
		virtual string to_string() const = 0;

		//returns nullptr on error
		virtual std::unique_ptr<parser_node> get_child() const = 0;
		virtual std::unique_ptr<parser_node> get_child(std::string_view) const = 0;

		//returns empty vector on error
		virtual std::vector<std::unique_ptr<parser_node>> get_children() const = 0;

		template<typename T, typename Converter = nullptr_t>
		T to_scalar(Converter conv = nullptr) const;

		template<typename T, typename Converter = nullptr_t>
		std::vector<T> to_sequence(Converter conv = nullptr) const;

		template<typename Value, typename Converter = nullptr_t>
		std::vector<std::pair<string, Value>> to_map(Converter conv = nullptr) const;

		template<typename Key, typename Value, typename KeyConv = nullptr_t, typename ValueConv = nullptr_t>
		std::vector<std::pair<Key, Value>> to_map(KeyConv k_conv = nullptr, ValueConv v_conv = nullptr) const;
	};

	namespace parse_tools
	{
		inline void log_parse_error(std::string_view resource_type, std::string_view resource_name,
			std::string_view property_name, std::string_view requested_type, hades::unique_id mod);

		template<class T, typename ConversionFunc>
		T get_scalar(const parser_node &node, std::string_view resource_type, std::string_view resource_name,
			std::string_view property_name, T default_value, hades::unique_id mod,ConversionFunc convert = nullptr);

		template<typename ConversionFunc>
		unique_id get_scalar(const parser_node &node, std::string_view resource_type, std::string_view resource_name,
			std::string_view property_name, unique_id default_value, hades::unique_id mod, ConversionFunc convert = nullptr);

		template<class T, typename ConversionFunc>
		T get_scalar(const parser_node &node, std::string_view resource_type, std::string_view resource_name,
			std::string_view property_name, T default_value, ConversionFunc convert = nullptr);
	}
}

#include "hades/detail/parser.inl"

#endif // !HADES_PARSER_HPP
