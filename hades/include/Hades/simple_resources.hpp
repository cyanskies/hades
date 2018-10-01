#ifndef HADES_SIMPLERESOURCE_HPP
#define HADES_SIMPLERESOURCE_HPP

#include <variant>

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/Shader.hpp"

#include "hades/curve.hpp"
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
		struct texture;

		struct string : public resource_type<types::string>
		{};

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
			unique_id vert_mod = unique_id::zero,
				geo_mod = unique_id::zero;
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
