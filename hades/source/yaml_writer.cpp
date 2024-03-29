#include "hades/yaml_writer.hpp"

#include <cassert>

#include "yaml-cpp/emitter.h"

#include "hades/types.hpp"

namespace
{
	class yaml_writer final : public hades::data::writer
	{
	public:
		yaml_writer() noexcept 
		{
			_emitter << YAML::BeginMap;
		}

		yaml_writer(std::ostream& o) noexcept
			: _emitter{ o }
		{
			_emitter << YAML::BeginMap;
		}

		void start_sequence() override
		{
			_emitter << YAML::Flow << YAML::BeginSeq;
		}

		void start_sequence(std::string_view key) override
		{
			_emitter << YAML::Key << hades::to_string(key);
			_emitter << YAML::Value << YAML::BeginSeq;
		}

		void end_sequence() override
		{
			_emitter << YAML::EndSeq;
		}

		void start_map() override
		{
			_emitter << YAML::BeginMap;
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

		hades::string get_string() const override
		{
			auto strm = std::ostringstream{};
			print(strm);
			return strm.str();
		}

		void print(std::ostream& os) const override
		{
			assert(_emitter.good());
			os << _emitter.c_str() << '\n';
			//manually add the final newline(emitter sometimes misses it)
			//no harm in having it twice
			return;
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