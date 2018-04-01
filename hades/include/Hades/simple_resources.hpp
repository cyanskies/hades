#ifndef HADES_SIMPLERESOURCE_HPP
#define HADES_SIMPLERESOURCE_HPP

#include <variant>

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/Shader.hpp"

#include "Hades/Curves.hpp"
#include "Hades/parallel_jobs.hpp"
#include "Hades/resource_base.hpp"
#include "Hades/Types.hpp"

//This header provides resources for built in types

namespace hades
{
	namespace data
	{
		class data_system;
	}

	void RegisterCommonResources(hades::data::data_system*);

	template<class First, class Last>
	sf::Color MakeColour(First begin, Last end)
	{
		auto count = std::distance(begin, end);
		if (count < 3 || count > 4)
		{
			//TODO: log failure
			return sf::Color::Magenta;
		}

		static auto min = 0;
		static auto max = 255;

		auto r = std::clamp(*begin++, min, max);
		auto g = std::clamp(*begin++, min, max);
		auto b = std::clamp(*begin++, min, max);
		decltype(r) a = 255;

		if (count == 4)
			a = std::clamp(*begin++, min, max);

		assert(begin == end);

		return sf::Color(r, g, b, a);
	}

	namespace resources
	{
		struct texture : public resource_type<sf::Texture>
		{
			texture();

			using size_type = types::uint16;
			//max texture size for older hardware is 512
			//max size for modern hardware is 8192 or higher
			size_type width = 0, height = 0;
			bool smooth = false, repeat = false, mips = false;
		};

		struct string : public resource_type<types::string>
		{};

		//a system stores a job function
		struct system : public resource_type<parallel_jobs::job_function>
		{
			//if loaded from a manifest then it should be loaded from scripts
			//if it's provided by the application, then source is empty, and no laoder function is provided.
		};

		enum VariableType {ERROR, INT, FLOAT, BOOL, STRING, OBJECT_REF, UNIQUE, VECTOR_INT, VECTOR_FLOAT, VECTOR_OBJECT_REF, VECTOR_UNIQUE};

		namespace curve_types
		{
			using int_t = hades::types::int32;
			using float_t = float;
			using bool_t = bool;
			using string = hades::types::string;
			using object_ref = int; //? entity ID
			using unique = hades::data::UniqueId;
			using vector_int = std::vector<int_t>;
			using vector_float = std::vector<float_t>;
			using vector_object_ref = std::vector<object_ref>;
			using vector_unique = std::vector<unique>;
		}

		struct curve_default_value
		{
			bool set = false;

			using int_t = hades::types::int32;
			using curve_value = std::variant<int_t, float,
				bool, hades::types::string, hades::data::UniqueId, std::vector<int_t>,
				std::vector<float>, std::vector<hades::data::UniqueId>>;

			curve_value value;
		};

		bool operator==(const curve_default_value &lhs, const curve_default_value &rhs);

		struct curve_t {};

		struct curve : public resource_type<curve_t>
		{
			CurveType curve_type = CurveType::ERROR;
			VariableType data_type = VariableType::ERROR;
			bool sync = false,
				save = false,
				trim = false;
			curve_default_value default_value;
		};

		types::string CurveValueToString(curve_default_value);
		curve_default_value StringToCurveValue(const curve*, std::string_view str);

		struct shader : public resource_type<sf::Shader>
		{
			//enum class uniform_type {
			//	FLOAT, // a floating point value
			//	VEC2, //a vector2f
			//	VEC3, //vector3f
			//	VEC4, //vector4f
			//	INT, //Interger value
			//	IVEC2, //a vector2i
			//	IVEC3, //vector3i
			//	IVEC4, //vector4i
			//	BOOL, //boolean value
			//	BVEC2, //a vector of 2 bools
			//	BVEC3, //vector3b
			//	BVEC4, //vector4b
			//	MAT3, //a float 3x3 matrix
			//	MAT4, //4x4 float matrix
			//	SAMPLER2D, //a sf::texture
			//	SAMPLE_CURRENT, //sets the currently bound texture as a SAMPLER2D
			//	//Array types
			//	ARRAY_FLOAT,
			//	ARRAY_VEC2,
			//	ARRAY_VEC3,
			//	ARRAY_VEC4,
			//	ARRAY_MAT3,
			//	ARRAY_MAT4
			//};
			////map of uniforms used by the shader
			////pairs of name to uniform type
			//using uniform_entry = std::tuple<types::string, uniform_type>;
			//std::vector<uniform_entry> uniforms;

			types::string vert_source, geo_source;
			data::UniqueId vert_mod = data::UniqueId::Zero,
				geo_mod = data::UniqueId::Zero;
			//source and mod refeer to the fragment shader source and mod

			template<typename T>
			static constexpr bool valid_type()
			{
				return false;
			}
		};

		template<>
		constexpr bool shader::valid_type<float>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<sf::Glsl::Vec2>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<sf::Glsl::Vec3>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<sf::Glsl::Vec4>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<int>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<sf::Glsl::Ivec2>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<sf::Glsl::Ivec3>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<sf::Glsl::Ivec4>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<bool>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<sf::Glsl::Bvec2>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<sf::Glsl::Bvec3>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<sf::Glsl::Bvec4>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<sf::Glsl::Mat3>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<sf::Glsl::Mat4>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<const sf::Texture*>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<sf::Shader::CurrentTextureType>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<std::vector<float>>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<std::vector<sf::Glsl::Vec2>>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<std::vector<sf::Glsl::Vec3>>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<std::vector<sf::Glsl::Vec4>>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<std::vector<sf::Glsl::Mat3>>()
		{
			return true;
		}

		template<>
		constexpr bool shader::valid_type<std::vector<sf::Glsl::Mat4>>()
		{
			return true;
		}

		struct animation_frame
		{
			//the rectangle for this frame and the duration relative to the rest of the frames in this animation
			animation_frame() = default;
			animation_frame(types::uint16 nx, types::uint16 ny, float nd) : x(nx), y(ny), duration(nd) {}
			types::uint16 x = 0, y = 0;
			float duration = 1.f;
		};

		//TODO: add field for fragment shaders
		struct animation : public resource_type<std::vector<animation_frame>>
		{
			animation();

			const texture* tex = nullptr;
			float duration = 1.f;
			types::int32 width = 0, height = 0;
			const shader *anim_shader = nullptr;
		};

		struct font : public resource_type<sf::Font>
		{
			font();
		};
	}
}

#endif //HADES_SIMPLERESOURCE_HPP
