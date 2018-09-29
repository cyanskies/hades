#include "hades/yaml_parser.hpp"

#include "yaml-cpp/yaml.h"

#include "hades/logging.hpp"
#include "hades/parser.hpp"
#include "hades/types.hpp"
#include "hades/utility.hpp"

namespace hades::data
{
	class yaml_parser_node : public parser_node
	{
	public:
		enum class yaml_type {MAP, SEQUENCE, SCALAR};

		explicit yaml_parser_node(YAML::Node node) 
			: _nodes(std::move(node), YAML::Node{}) 
		{
			if (_nodes.first.IsSequence())
				_type = yaml_type::SEQUENCE;
		}

		yaml_parser_node(YAML::Node first, YAML::Node second) 
			: _nodes(std::make_pair(std::move(first), std::move(second))), _type(yaml_type::MAP) {}

		string to_string() const override
		{
			try
			{
				//this should work reguardless of yaml_type
				return _nodes.first.as<string>();
			}
			catch (const YAML::Exception &e)
			{
				throw parser_convert_exception(e.what());
			}
		}

		std::unique_ptr<parser_node> get_child() const override
		{
			auto seq = get_children();
			if (seq.size() > 1)
				LOGWARNING("Called get_child on a data node: " + to_string() + ", but that node has more than one child.");

			if (seq.empty())
				return nullptr;

			return std::move(*seq.begin());
		}

		std::unique_ptr<parser_node> get_child(std::string_view name) const override
		{
			try
			{
				if(_type == yaml_type::MAP)
					return std::make_unique<yaml_parser_node>(_nodes.second[hades::to_string(name)]);
				
				return std::make_unique<yaml_parser_node>(_nodes.first[hades::to_string(name)]);
			}
			catch (const YAML::Exception&)
			{
				return nullptr;
			}
		}

		std::vector<std::unique_ptr<parser_node>> get_children() const override
		{
			std::vector<std::unique_ptr<parser_node>> result;

			try
			{
				const auto &node = _type == yaml_type::MAP ? _nodes.second : _nodes.first;
				for (const auto &n : node)
				{
					if(node.IsMap())
						result.emplace_back(std::make_unique<yaml_parser_node>(n.first, n.second));
					else
						result.emplace_back(std::make_unique<yaml_parser_node>(n));
				}
			}
			catch (YAML::Exception&)
			{
				result.clear();
			}

			return result;
		}

	private:
		std::pair<YAML::Node, YAML::Node> _nodes;
		yaml_type _type = yaml_type::SCALAR;
	};

	std::unique_ptr<parser_node> make_yaml_parser(std::string_view src)
	{
		try
		{
			return std::make_unique<yaml_parser_node>(YAML::Load(to_string(src)));
		}
		catch (const YAML::ParserException &p)
		{
			throw yaml_parse_exception(p.what());
		}
	}

	std::unique_ptr<parser_node> make_yaml_parser(const YAML::Node &src)
	{
		return std::make_unique<yaml_parser_node>(src);
	}
}