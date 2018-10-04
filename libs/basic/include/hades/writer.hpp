#ifndef HADES_WRITER_HPP
#define HADES_WRITER_HPP

#include <string>
#include <string_view>

#include "hades/utility.hpp"

namespace hades::data
{
	class write_error : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	//data writer interface
	class writer
	{
	public:
		virtual void start_sequence(std::string_view) = 0;
		virtual void end_sequence() = 0;
		virtual void start_map(std::string_view) = 0;
		virtual void end_map() = 0;
		virtual void write(std::string_view) = 0;
		virtual void write(std::string_view key, std::string_view value) = 0;

		virtual std::string get_string() const = 0;

		template<typename T>
		void start_map(T value)
		{
			start_map(to_string(value));
		}

		template<typename T, typename U>
		void write(T key, U value)
		{
			write(to_string(key), to_string(value));
		}

		template<typename T>
		void write(T value)
		{
			write(to_string(value));
		}
	};
}

//start_map (terrain_yaml)
e << YAML::Key << terrain_yaml;

e << YAML::Value << YAML::BeginMap;


//start_sequence(terrainset)
//write terrains
e << YAML::Key << terrainset_yaml;
e << YAML::Value << YAML::Flow;
e << YAML::BeginSeq;

//wrtie(t);
for (const auto &t : l.terrain.terrainset)
e << t;

//end_sequence
e << YAML::EndSeq;

//write each tile layer
e << YAML::Key << tile_yaml;

#endif // !HADES_WRITER_HPP
