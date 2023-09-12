#include "hades/shader.hpp"

#include "hades/files.hpp"
#include "hades/parser.hpp"

using namespace std::string_view_literals;
constexpr auto shader_str = "shaders"sv;

namespace hades::resources
{
	enum class shaders_provided : std::uint8_t {
		none = 0,
		vert = 1,
		geo = 2,
		frag = 4,
		vert_frag = vert | frag,
		all = vert | geo | frag
	};

	shaders_provided operator|=(shaders_provided& lhs, shaders_provided rhs)
	{
		return lhs = static_cast<shaders_provided>(enum_type(lhs) | enum_type(rhs));
	}

	struct shader_t {};

	struct shader : public resource_type<shader_t>
	{
		shader() = default;
		// NOTE: Custom copy contructor needed because sf::Shader is non-copyable;
		//		 and proxy holds ptrs to this object and the uniforms subobject
		shader(const shader& rhs)
			: vertex{ rhs.vertex }, geometry{ rhs.geometry }, fragment{ rhs.fragment },
			uniforms{ rhs.uniforms }
		{
			loaded = false;
			return;
		}

		void load(data::data_manager&) final override
		{
			auto shaders = shaders_provided::none;
			if (!empty(vertex))
				shaders |= shaders_provided::vert;
			if (!empty(geometry))
				shaders |= shaders_provided::geo;
			if (!empty(fragment))
				shaders |= shaders_provided::frag;

			auto ret = false;
			switch (shaders)
			{
			case shaders_provided::vert:
				ret = sf_shader.loadFromMemory(vertex, sf::Shader::Vertex);
				break;
			case shaders_provided::geo:
				ret = sf_shader.loadFromMemory(geometry, sf::Shader::Geometry);
				break;
			case shaders_provided::frag:
				ret = sf_shader.loadFromMemory(fragment, sf::Shader::Fragment);
				break;
			case shaders_provided::vert_frag:
				ret = sf_shader.loadFromMemory(vertex, fragment);
				break;
			case shaders_provided::all:
				ret = sf_shader.loadFromMemory(vertex, geometry, fragment);
				break;
			default:
				log_error("Failed to load shader, incompatible combination of programs"sv);
				return;
			}

			if (!ret)
			{
				log_error("Failed to load shader, error in provided programs"sv);
				return;
			}

			proxy.emplace(this, &uniforms);
			loaded = true;
			return;
		}

		//void serialise(const data::data_manager&, data::writer&) const override;

		string vertex, geometry, fragment;
		shader_uniform_map uniforms;
		sf::Shader sf_shader;
		std::optional<shader_proxy> proxy;
	};

	shader_proxy::shader_proxy(shader* s, const shader_uniform_map* u)
		: _shader{ s }, _uniforms{ u }
	{
		assert(s);
		assert(u);
		_uniform_set.resize(size(*_uniforms), false);
		return;
	}

	struct uniform_visitor
	{
		shader_proxy& proxy;
		std::string_view name;

		void operator()(std::monostate)
		{}

		void operator()(const auto& value)
		{
			proxy.set_uniform(name, value);
			return;
		}
	};

	void shader_proxy::start_uniforms()
	{
		auto iter = begin(*_uniforms);
		const auto end = std::end(*_uniforms);

		auto iter2 = begin(_uniform_set);
		const auto end2 = std::end(_uniform_set);
		assert(size(*_uniforms) == size(_uniform_set));

		while (iter != end)
		{
			if (!std::holds_alternative<std::monostate>(iter->second.value))
			{
				std::visit(uniform_visitor{ *this, iter->first }, iter->second.value);
				*iter2 = true;
			}

			++iter;
			++iter2;
		}

		return;
	}

	void shader_proxy::end_uniforms() const
	{
		auto iter = begin(_uniform_set);
		const auto end = std::end(_uniform_set);
		while (iter != end)
		{
			if (!*iter)
			{
				const auto dist = distance(begin(_uniform_set), iter);
				auto iter2 = std::next(begin(*_uniforms), dist);
				throw uniform_not_set{ std::format("Shader cannot be used, render system didn't set uniform: {}", iter2->first) };
			}
			++iter;
		}

		return;
	}

	const sf::Shader& shader_proxy::get_shader() const
	{
		return _shader->sf_shader;
	}
}

namespace hades::resources::shader_functions
{
	const shader* get_resource(unique_id id)
	{
		return data::get<shader>(id);
	}

	shader* get_resource(data::data_manager& d, unique_id id, std::optional<unique_id> mod)
	{
		return d.get<shader>(id, mod);
	}

	resource_link<shader> make_resource_link(data::data_manager& d, unique_id id, unique_id from)
	{
		return d.make_resource_link<shader>(id, from, [](data::data_manager& d, unique_id target)->const shader* {
			return d.get<shader>(target, data::no_load);
			});
	}

	try_get_return try_get(unique_id id) noexcept
	{
		return data::try_get<shader>(id);
	}

	shader* find_or_create(data::data_manager& d, unique_id id, std::optional<unique_id> mod)
	{
		return d.find_or_create<shader>(id, mod, shader_str);
	}

	bool is_loaded(const shader& s) noexcept
	{
		return s.loaded;
	}

	unique_id get_id(const shader& s) noexcept
	{
		return s.id;
	}

	std::vector<unique_id> get_id(const std::vector<const shader*>& shaders)
	{
		auto ret = std::vector<unique_id>{};
		ret.reserve(size(shaders));
		std::ranges::transform(shaders, back_inserter(ret), [](const shader* a) {
			assert(a);
			return a->id;
			});
		return ret;
	}

	resource_base* get_resource_base(shader& s) noexcept
	{
		return &s;
	}

	sf::Shader& get_shader(shader& s) noexcept
	{
		return s.sf_shader;
	}

	const shader_uniform_map& get_uniforms(const shader& s) noexcept
	{
		return s.uniforms;
	}

	shader_proxy get_shader_proxy(const shader& s)
	{
		try
		{
			return s.proxy.value();
		}
		catch (const std::bad_optional_access&)
		{
			throw shader_error{ "Tried to access the shader for an animation that has no shader" };
		}
	}

	void set_vertex(shader& s, std::string vert) noexcept
	{
		s.vertex = std::move(vert);
		s.loaded = false;
		return;
	}

	void set_geometry(shader& s, std::string geo) noexcept
	{
		s.geometry = std::move(geo);
		s.loaded = false;
		return;
	}

	void set_fragment(shader& s, std::string frag) noexcept
	{
		s.fragment = std::move(frag);
		s.loaded = false;
		return;
	}

	void set_uniforms(shader& s, shader_uniform_map map) noexcept
	{
		s.uniforms = std::move(map);
		s.loaded = false;
		return;
	}
}

namespace hades
{
	static constexpr resources::uniform_type_list uniform_type_from_string(std::string_view s)
	{
		constexpr auto type_strings = std::array{
			"float"sv, "vec2"sv, "vec3"sv, "vec4"sv,
			"int"sv, "ivec2"sv, "ivec3"sv, "ivec4"sv,
			"mat3"sv, "mat4"sv,
			"texture"sv
		};

		using resources::uniform_type_list;
		static_assert(size(type_strings) == enum_type(uniform_type_list::end));

		constexpr auto end = size(type_strings);
		for (auto i = std::underlying_type_t<uniform_type_list>{}; i < end; ++i)
		{
			if (type_strings[i] == s)
				return uniform_type_list{ i };
		}

		return uniform_type_list::end;
	}

	static string get_shader_program_source(const hades::data::parser_node& shdr,
		std::string_view program_type, std::string_view name, const data::mod mod)
	{
		const auto program = shdr.get_child(program_type);
		if (program)
		{
			const auto prog_path_node = program->get_child();
			if (prog_path_node)
			{
				const auto prog_path = prog_path_node->to_string();
				try
				{
					return files::read_resource(mod, prog_path);
				}
				catch (const files::file_not_found& e)
				{
					log_error(std::format("Unable to find {} program source for shader: {}; message was: {}"sv, program_type, name, e.what()));
					return {};
				}
			}

			log_warning(std::format("Expected path for shader {} program while parsing shader: {}"sv, program_type, name));
		}

		return {}; // unprovided is also fine
	}

	void parse_shaders(hades::unique_id mod, const hades::data::parser_node& node, hades::data::data_manager& d)
	{
		//shaders:
		//	team-colour-shader:
		//		vertex: path or source
		//		fragment: path or source
		//		geometry: path or source
		//		uniforms:
		//			name:
		//				type: int
		//				value: 50

		const auto& mod_res = d.get_mod(mod);
		using namespace resources;

		for (const auto& shader_node : node.get_children())
		{
			log_warning("Shader parser has not been tested"sv);

			const auto name = shader_node->to_string();
			const auto id = d.get_uid(name);

			auto shdr = shader_functions::find_or_create(d, id, mod);

			auto shader_missing_count = uint8{};

			auto program_source = get_shader_program_source(*shader_node, "vertex"sv, name, mod_res);
			if (!empty(program_source))
				shader_functions::set_vertex(*shdr, std::move(program_source));
			else
				++shader_missing_count;

			program_source = get_shader_program_source(*shader_node, "geometry"sv, name, mod_res);
			if (!empty(program_source))
				shader_functions::set_geometry(*shdr, std::move(program_source));
			else
				++shader_missing_count;

			program_source = get_shader_program_source(*shader_node, "fragment"sv, name, mod_res);
			if (!empty(program_source))
				shader_functions::set_fragment(*shdr, std::move(program_source));
			else
				++shader_missing_count;

			if (shader_missing_count > 2)
				log_error(std::format("Error parsing shader: {}, no programs provided"sv, name));

			//uniforms
			if (const auto uniforms_node = shader_node->get_child("uniforms"sv); uniforms_node)
			{
				auto uniforms = hades::detail::parse_shader_uniform_defaults(*uniforms_node, d, id);
				shader_functions::set_uniforms(*shdr, std::move(uniforms));
			}
			else
				log_warning(std::format("expected uniforms when parsing shader: {}"sv, name));
		}

		return;
	}

	void register_shader_resource(data::data_manager& d)
	{
		// TODO: parser for shader types
		d.register_resource_type(shader_str, nullptr);
		return;
	}
}

namespace hades::detail
{
	template<typename T>
	struct matrix_size;

	template<std::size_t N>
	struct matrix_size<sf::priv::Matrix<N, N>> : std::integral_constant<std::size_t, N>
	{};

	struct parse_uniform_visitor
	{
		resources::uniform_variant& value;
		data::parser_node& node;
		data::data_manager& data;
		unique_id source;

		template<typename T>
		void operator()();

		template<typename T>
			requires std::is_arithmetic_v<T>
		void operator()() // int + float
		{
			// value: 5
			// value: 5.f
			const auto val = node.get_child();
			value = val->to_scalar<T>();
			return;
		}

		template<typename T>
			requires is_basic_vector_v<T>
		void operator()()
		{
			// value: [1, 2]
			using Ty = typename T::value_type;
			constexpr auto size = basic_vector_size_v<T>;
			const auto values = node.to_sequence<Ty>();
			if (values.size() != size)
			{
				log_error("uniform value invalid, wrong number of elements for vec type"sv);
				return;
			}
			auto out = T{ values[0], values[1] };
			if constexpr (size > 2)
				out.z = values[2];
			if constexpr (size > 3)
				out.w = values[3];
			value = out;
			return;
		}	

		template<typename T>
			requires std::same_as<T, sf::Glsl::Mat3> || std::same_as<T, sf::Glsl::Mat4>
		void operator()()
		{
			const auto values = node.to_sequence<float>();
			constexpr auto size = matrix_size<T>::value;
			if (values.size() != size * size)
			{
				log_error("uniform value invalid, wrong number of elements for matrix type"sv);
				return;
			}

			value = T{ std::data(values) };
			return;
		}

		template<>
		void operator()<resources::resource_link<resources::texture>>()
		{
			auto val = node.get_child();
			if (!val)
			{
				log_error("uniform value invalid, name missing for texture"sv);
				return;
			}

			const auto str = val->to_string();
			if (str == "current-texture"sv)
				value = sf::Shader::CurrentTexture;
			else
			{
				const auto id = data.get_uid(str);
				value = resources::texture_functions::make_resource_link(data, id, source);
			}
			return;
		}
	};

	template<bool RequireValue = false>
	resources::shader_uniform_map parse_shader_uniform_defaults_impl(
		const data::parser_node& shader_uniforms, data::data_manager& d, unique_id source)
	{
		//	shader-uniforms:
		//		name:
		//			type:
		//			value:

		using namespace resources;
		auto uniforms = shader_uniform_map{};

		for (const auto& uniform_node : shader_uniforms.get_children())
		{
			const auto name = uniform_node->to_scalar<unique_id>();
			const auto type = data::parse_tools::get_scalar(*uniform_node,
				"type"sv, uniform_type_list::end, uniform_type_from_string);
			if (type == uniform_type_list::end)
			{
				log_error(std::format("Invalid type while parsing uniform: {}", d.get_as_string(name)));
				continue;
			}

			auto value = uniform_variant{};

			const auto value_node = uniform_node->get_child("value"sv);
			if (!value_node)
			{
				if constexpr (RequireValue)
				{
					log_error(std::format("Invalid value while parsing uniform: {}", d.get_as_string(name)));
					continue;
				}
			}
			else
				call_with_uniform_type(type, parse_uniform_visitor{ value, *value_node, d, source });

			uniforms.emplace(d.get_as_string(name), uniform{ type, std::move(value) });
		}

		return uniforms;
	}

	resources::shader_uniform_map parse_shader_uniform_defaults(
		const data::parser_node& shader_uniforms, data::data_manager& d, unique_id source)
	{
		return parse_shader_uniform_defaults_impl<true>(shader_uniforms, d, source);
	}
}
