#include "hades/data_system.hpp"

#include <span>

#include "hades/files.hpp"
#include "hades/writer.hpp"

namespace hades::data
{
	namespace detail
	{
		inline void serialise_mod(const mod& m, writer& w, std::span<std::filesystem::path> includes)
		{
			//mod:
			//	name: ortho
			//	depends:
			//		- yep
			//		- another

			//include:
			//	- rts.yaml
			//	- terrain/terrain.yaml
			//	- units/units.yaml

			using namespace std::string_view_literals;

			w.start_map("mod"sv);
			w.write("name"sv, m.name);
			if (!empty(m.dependencies))
			{
				w.start_sequence("depends"sv);
				for (auto& d : m.dependencies)
					w.write(data::get_as_string(d));
				w.end_sequence();
			}
			w.end_map();

			if (!empty(includes))
			{
				w.start_sequence("include"sv);
				for (auto& i : includes)
					w.write(i.generic_string());
				w.end_sequence();
			}

			return;
		}

		struct resource_pair
		{
			std::string_view type;
			const resources::resource_base* res;
		};
	}

	template<typename Stream>
	void data_system::_write_mod(const mod& m, const std::filesystem::path& root, Stream& strm)
	{
		using namespace std::string_view_literals;
		// we use a std stream for normal file writing
		// and out oafstream for writing archives
		constexpr auto std_stream = std::same_as<Stream, std::ofstream>;
		const auto& mod_storage = get_mod_data(m.id);
		auto queue = std::vector<detail::resource_pair>{};
		auto includes = std::vector<std::filesystem::path>{};

		for (const auto& [name, resources] : mod_storage.resources_by_type)
		{
			for (const auto res : resources)
			{
				queue.emplace_back(name, res);
				if (std::ranges::find(includes, res->data_file) == end(includes))
					includes.emplace_back(res->data_file);
			}
		}

		const auto game_yaml = std::filesystem::path{ "game.yaml"sv };
		const auto mod_yaml = std::filesystem::path{ "mod.yaml"sv };

		std::erase(includes, game_yaml);
		std::erase(includes, mod_yaml);

		//queue is currently sorted by resource type
		// sort by data file then resource type
		std::ranges::sort(queue, [](auto&& left, auto&& right) noexcept {
			return std::tie(left.res->data_file, left.type) <
				std::tie(right.res->data_file, right.type);
			});
		
		const std::filesystem::path* data_file = {};
		auto res_type = std::string_view{};
		auto data_output = std::unique_ptr<writer>{};

		const auto close_writers = [&data_output, &strm]() {
			if (data_output)
			{
				data_output->end_map();
				strm << data_output;
			}

			data_output.reset();

			if (strm.is_open())
				strm.close();
		};

		// write all the data files out
		for (const auto& [type, res] : queue)
		{
			// change data file
			if (!data_file || res->data_file != *data_file)
			{
				std::invoke(close_writers);

				res_type = {};
				data_file = &res->data_file;

				if constexpr (std_stream)
				{
					// make dir if needed
					auto path = root / *data_file;
					path.remove_filename();
					try 
					{
						std::filesystem::create_directories(path);
					}
					catch (const std::filesystem::filesystem_error& e)
					{
						throw files::file_error{ e.what() };
					}
				}

				if constexpr (std_stream)
					strm.open(root / *data_file, std::ios_base::trunc);
				else
					strm.open(root / *data_file);

				if (!strm.good())
					throw files::file_error{ "Error creating data file" };

				data_output = make_writer();

				log_debug("Writing: " + (root / *data_file).generic_string());

				if (const auto stem = data_file->stem();
					stem == "mod"sv || stem == "game"sv)
				{
					detail::serialise_mod(m, *data_output, includes);
				}
			}

			// change the current resource type
			if (res_type != type)
			{
				if (res_type != std::string_view{})
					data_output->end_map();

				data_output->start_map(type);
				res_type = type;
			}

			res->serialise(*this, *data_output);
		}

		// write out actual resources
		std::invoke(close_writers);
		for (const auto& [type, res] : queue)
		{
			if (res->serialise_source())
			{
				const auto& loaded = res->loaded;
				if (!loaded)
					load(res->id);

				if (!loaded || (res->loaded_path.empty() && res->loaded_archive_path.empty()))
				{
					log_warning("Failed to load: " + res->source.generic_string() + " skipping file");
					continue;
				}

				const auto path = root / res->source;

				if constexpr (std_stream)
				{
					auto dir = path;
					dir.remove_filename();
					std::filesystem::create_directories(dir);
				}

				log_debug("Writing: " + path.generic_string());
				if constexpr (std_stream)
					strm.open(path, std::ios_base::binary | std::ios_base::trunc);
				else
					strm.open(path);

				res->serialise(strm);
				strm.close();
			}
		}

		return;
	}
}
