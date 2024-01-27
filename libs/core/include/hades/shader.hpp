#ifndef HADES_SHADER_HPP
#define HADES_SHADER_HPP

#include <tuple>
#include <variant>

#include "SFML/Graphics/Shader.hpp"

#include "hades/data.hpp"
#include "hades/string.hpp"
#include "hades/texture.hpp"
#include "hades/vector_math.hpp"

// Shaders are defined as a resource
// they specify the programs they define and the uniforms (and default values) they need
// 
// Then animations can list a shader by resource and provide their own default values for the uniforms

namespace hades::resources
{
	class shader_error : public data::resource_error
	{
	public:
		using resource_error::resource_error;
	};

	class unexpected_uniform : public shader_error
	{
	public:
		using shader_error::shader_error;
	};

	class uniform_wrong_type : public shader_error
	{
	public:
		using shader_error::shader_error;
	};

	class uniform_not_set : public shader_error
	{
	public:
		using shader_error::shader_error;
	};

	// shader uniform types
	enum class uniform_type_list : uint8 {
		float_t, begin = float_t, vector2f, vector3f, vector4f,
		int_t, vector2i, vector3i, vector4i,
		matrix3, matrix4,
		texture, // current_texture
		end
	};

	string uniform_type_to_string(uniform_type_list);

	template<typename Func>
	void call_with_uniform_type(uniform_type_list, Func&& f);
	template<typename T>
	uniform_type_list get_uniform_type_enum();

	using uniform_type_pack = std::tuple<
		float,
		vector2_float,
		vector3<float>,
		vector4<float>,
		int,
		vector2_int,
		vector3<int>,
		vector4<int>,
		sf::Glsl::Mat3,
		sf::Glsl::Mat4,
		resource_link<texture>,
		sf::Texture*, // non-resource texture
		sf::Shader::CurrentTextureType // render state texture
	>;

	template<typename T>
	concept uniform_type = requires(uniform_type_pack u)
	{
		std::get<T>(u);
	};

	namespace detail
	{
		template<typename T>
		struct variant_from_tuple;

		template<typename ...Ts>
		struct variant_from_tuple<std::tuple<Ts...>>
		{
			using type = std::variant<std::monostate, Ts...>;
		};

		template<std::size_t Columns, std::size_t Rows>
		bool matrix_eq(const sf::priv::Matrix<Columns, Rows>& lhs, const sf::priv::Matrix<Columns, Rows>& rhs)
		{
			return std::equal(std::begin(lhs.array), std::end(lhs.array),
				std::begin(rhs.array));
		}
	}

	using uniform_variant = detail::variant_from_tuple<uniform_type_pack>::type;

	struct uniform
	{
		uniform_type_list type = uniform_type_list::end;
		uniform_variant value;
	};

	inline bool operator==(const uniform& lhs, const uniform& rhs) noexcept
	{
		if (lhs.type == rhs.type
			&& lhs.value.index() == rhs.value.index())
		{
			return std::visit([]<typename Ty, typename Ty2>(const Ty & lhs, const Ty2 & rhs) noexcept {
				using T = std::decay_t<Ty>;
				using T2 = std::decay_t<Ty2>;
				//equality tests for sfml types that don't have quality operators
				if constexpr (!std::same_as<T, T2>)
					return false;
				else if constexpr (std::same_as<T, sf::Glsl::Mat3> ||
					std::same_as<T, sf::Glsl::Mat4>)
				{
					return detail::matrix_eq(lhs, rhs);
				}
				else if constexpr (std::same_as<T, sf::Shader::CurrentTextureType>
					|| std::same_as<T, std::monostate>)
				{
					return true;
				}
				else
					return lhs == rhs;
			}, lhs.value, rhs.value);
		}
		
		return false;
	}

	using shader_uniform_map = map_string<uniform>;
	
	struct shader; // fwd

	// create a shader_instance to render with a shader
	class shader_proxy
	{
	public:
		shader_proxy(shader* s, shader_uniform_map u);
		// THROWS: unexpected_uniform, uniform_wrong_type
		template<uniform_type T>
		void set_uniform(std::string_view, T);
		void set_uniforms(const shader_uniform_map&);

		const sf::Shader* get_shader() const;

		bool operator==(const shader_proxy& rhs) const noexcept
		{
			return _shader == rhs._shader
				&& _uniforms == rhs._uniforms;
		}

	private:
		shader_uniform_map _uniforms;
		mutable shader* _shader;
	};
	
	namespace shader_functions
	{
		using try_get_return = data::data_manager::try_get_return<const shader>;
		const shader* get_resource(unique_id);
		shader* get_resource(data::data_manager&, unique_id, std::optional<unique_id> = {});
		resource_link<shader> make_resource_link(data::data_manager&, unique_id, unique_id from);
		try_get_return try_get(unique_id) noexcept;
		shader* find_or_create(data::data_manager&, unique_id, std::optional<unique_id> mod = {});
		bool is_loaded(const shader&) noexcept;
		unique_id get_id(const shader&) noexcept;
		std::vector<unique_id> get_id(const std::vector<const shader*>&);
		resource_base* get_resource_base(shader&) noexcept;

		// returns the underlying sf::Shader
		sf::Shader& get_shader(shader&) noexcept;
		const shader_uniform_map& get_uniforms(const shader&) noexcept;
		// THROWS: shader_error if the shader hasnt been loaded
		shader_proxy get_shader_proxy(const shader&);

		void set_vertex(shader&, std::string) noexcept;
		void set_geometry(shader&, std::string) noexcept;
		void set_fragment(shader&, std::string) noexcept;
		void set_uniforms(shader&, shader_uniform_map) noexcept;
	}
}

namespace hades
{
	void register_shader_resource(data::data_manager&);

	// to_string(matrix)
	// to_string(current_tex_type)
	// to_string(shdr)
}

namespace hades::detail
{
	// parses the shader uniform default values, as listed as part of an animation
	resources::shader_uniform_map parse_shader_uniform_defaults(const data::parser_node&, data::data_manager&, unique_id res);
}

#include "hades/detail/shader.inl"

#endif //HADES_SHADER_HPP
