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

		template<typename T, typename Converter>
		T to_scalar(Converter conv) const;

		template<typename T>
		T to_scalar() const
		{
			return to_scalar<T, nullptr_t>(nullptr);
		}

		//merges the stored sequence with the current sequence using the sequence operators
		// [=, elm1, elm2] replaces the current sequence with the new sequence
		// NOTE: = will cause the elements before it to be removed [elm1, =, elm2] == [elm2]
		// [+, elm1, elm2] adds the following elements to the sequence(is assumed by default
		// [-, elm1, elm2] removes elements equal to elm1 and elm2 are removed from the sequence
		template<typename T, typename Converter>
		std::vector<T> merge_sequence(std::vector<T> current, Converter conv) const;

		template<typename T>
		std::vector<T> merge_sequence(std::vector<T> current) const;

		template<typename T, typename Converter>
		std::vector<T> to_sequence(Converter conv) const;

		template<typename T>
		std::vector<T> to_sequence() const
		{
			return to_sequence<T, nullptr_t>(nullptr);
		}

		template<typename Value, typename Converter = nullptr_t>
		std::vector<std::pair<string, Value>> to_map(Converter conv = nullptr) const;

		template<typename Key, typename Value, typename KeyConv = nullptr_t, typename ValueConv = nullptr_t>
		std::vector<std::pair<Key, Value>> to_map(KeyConv k_conv = nullptr, ValueConv v_conv = nullptr) const;
	};

	using make_parser_f = std::unique_ptr<parser_node>(*)(std::string_view);

	void set_default_parser(make_parser_f);
	std::unique_ptr<parser_node> make_parser(std::string_view);

	namespace parse_tools
	{
		template<class T, typename ConversionFunc = nullptr_t>
		T get_scalar(const parser_node &node, std::string_view property_name,
			T default_value, ConversionFunc convert = nullptr);

		unique_id get_unique(const parser_node &node, std::string_view property_name, unique_id default_value);

		std::vector<unique_id> get_unique_sequence(const parser_node &node, std::string_view property_name,
			const std::vector<unique_id> &default_value);

		std::vector<unique_id> merge_unique_sequence(const parser_node &node, std::string_view property_name,
			const std::vector<unique_id> &current_value);

		template<class T, typename ConversionFunc = nullptr_t>
		std::vector<T> get_sequence(const parser_node &node, std::string_view property_name,
			const std::vector<T> &default_value, ConversionFunc convert = nullptr);

		template<class T, typename ConversionFunc = nullptr_t>
		std::vector<T> merge_sequence(const parser_node &node, std::string_view property_name,
			std::vector<T> current_value, ConversionFunc convert = nullptr);
	}
}

#include "hades/detail/parser.inl"

#endif // !HADES_PARSER_HPP
