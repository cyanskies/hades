#include "hades/yaml_writer.hpp"

#include "yaml-cpp/emitter.h"

namespace
{
	class yaml_writer : public hades::data::writer
	{
	public:
		yaml_writer() noexcept 
		{
			_emitter << YAML::BeginMap;
		}

		void start_sequence(std::string_view key) override
		{
			_emitter << YAML::Key << hades::to_string(key);
			_emitter << YAML::Value << YAML::Flow << YAML::BeginSeq;
		}

		void end_sequence() override
		{
			_emitter << YAML::EndSeq;
		}

		void start_map(std::string_view value) override
		{
			_emitter << YAML::Key << hades::to_string(value);
			_emitter << YAML::Value << YAML::BeginMap;
		}

		void end_map() override
		{
			_emitter << YAML::EndMap;
		}

		void write(std::string_view value) override
		{
			_emitter << hades::to_string(value);
		}

		void write(std::string_view key, std::string_view value) override
		{
			_emitter << YAML::Key << hades::to_string(key);
			_emitter << YAML::Value << hades::to_string(value);
		}

		std::string get_string() const override
		{
			return _emitter.c_str();
		}
	private:
		YAML::Emitter _emitter;
	};
}

namespace hades::data
{
	std::unique_ptr<writer> make_yaml_writer()
	{
		return std::make_unique<yaml_writer>();
	}
}