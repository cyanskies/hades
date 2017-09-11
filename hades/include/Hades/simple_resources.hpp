#ifndef HADES_SIMPLERESOURCE_HPP
#define HADES_SIMPLERESOURCE_HPP

#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/Shader.hpp"

#include "Hades/Curves.hpp"
#include "Hades/data_manager.hpp"
#include "Hades/parallel_jobs.hpp"
#include "Hades/Types.hpp"

//This header provides resources for built in types

namespace YAML
{
	class Node;
}

bool yaml_error(hades::types::string resource_type, hades::types::string resource_name,
	hades::types::string property_name, hades::types::string requested_type, hades::data::UniqueId mod, bool test);

namespace hades
{
	namespace resources
	{
		void parseTexture(data::UniqueId mod, YAML::Node& node, data::data_manager*);
		void loadTexture(resource_base* r, data::data_manager* dataman);
		void parseString(data::UniqueId mod, YAML::Node& node, data::data_manager*);
		void parseSystem(data::UniqueId mod, YAML::Node& node, data::data_manager*);
		void loadSystem(resource_base* r, data::data_manager* dataman);
		void parseCurve(data::UniqueId mod, YAML::Node& node, data::data_manager*);
		void parseAnimation(data::UniqueId mod, YAML::Node& node, data::data_manager*);
		void loadFont(resource_base* r, data::data_manager* data);
		void parseFont(data::UniqueId mod, YAML::Node& node, data::data_manager*);

		struct texture : public resource_type<sf::Texture>
		{
			texture() : resource_type<sf::Texture>(loadTexture) {}

			using size_type = types::uint16;
			//max texture size for older hardware is 512
			//max size for modern hardware is 8192 or higher
			size_type width, height;
			bool smooth, repeat, mips;
		};

		struct string : public resource_type<types::string>
		{};

		//a system stores a job function
		struct system : public resource_type<parallel_jobs::job_function>
		{
			//if loaded from a manifest then it should be loaded from scripts
			//if it's provided by the application, then source is empty, and no laoder function is provided.
		};

		enum VariableType {ERROR, INT, FLOAT, BOOL, STRING, VECTOR_INT, VECTOR_FLOAT};

		struct curve_t {};

		struct curve : public resource_type<curve_t>
		{
			CurveType curve_type;
			VariableType data_type;
			bool sync,
				save;
		};

		struct shader : public resource_type<sf::Shader>
		{
			enum class shader_type { VERTEX, GEOMETRY, FRAGMENT, SHADER_ERROR };
			shader_type type = SHADER_ERROR;
			enum class uniform_type {
				FLOAT, // a floating point value
				VEC2, //a vector2f
				VEC3, //vector3f
				VEC4, //vector4f
				INT, //Interger value
				IVEC2, //a vector2i
				IVEC3, //vector3i
				IVEC4, //vector4i
				BOOL, //boolean value
				BVEC2, //a vector of 2 bools
				BVEC3, //vector3b
				BVEC4, //vector4b
				MAT3, //a float 3x3 matrix
				MAT4, //4x4 float matrix
				SAMPLER2D, //a texture or other map
				SAMPLE_CURRENT, //sets the currently bound texture as a SAMPLER2D
				//Array types
				ARRAY_FLOAT,
				ARRAY_VEC2, 
				ARRAY_VEC3,
				ARRAY_VEC4,
				ARRAY_MAT3,
				ARRAY_MAT4
			};
			//map of uniforms used by the shader
			//pairs of name to uniform type
			using uniform_entry = std::tuple<types::string, uniform_type>;
			std::vector<uniform_entry> uniforms;
		};

		struct animation_frame
		{
			//the rectangle for this frame and the duration relative to the rest of the frames in this animation
			animation_frame() = default;
			animation_frame(types::uint16 nx, types::uint16 ny, float nd) : x(nx), y(ny), duration(nd) {}
			types::uint16 x, y;
			float duration;
		};

		//TODO: add field for fragment shaders
		struct animation : public resource_type<std::vector<animation_frame>>
		{
			texture* tex;
			float duration;
			types::int32 width, height;
			std::vector<data::UniqueId> shaders;
		};

		struct font : public resource_type<sf::Font>
		{
			font() : resource_type<sf::Font>(loadFont) {}
		};
	}
}

#endif //HADES_SIMPLERESOURCE_HPP
