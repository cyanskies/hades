#ifndef HADES_GRID_HPP
#define HADES_GRID_HPP

#include <vector>

#include "SFML/Graphics/Drawable.hpp"

#include "hades/colour.hpp"
#include "hades/vector_math.hpp"
#include "hades/vertex_buffer.hpp"

namespace hades
{
	class grid : public sf::Drawable
	{
	public:
		struct grid_properties
		{
			vector_float grid_size{};
			float cell_size{};
			colour line_colour{};
			float line_thickness{ 1.f };
		};

		void set_size(vector_float);
		void set_cell_size(float);
		void set_line_thickness(float);
		void set_colour(colour);
		void set_all(grid_properties);

		void draw(sf::RenderTarget&, sf::RenderStates = sf::RenderStates{}) const override;

	private:
		grid_properties _properties;
		vertex_buffer _verticies{ sf::PrimitiveType::Triangles, sf::VertexBuffer::Usage::Static };
	};
}

#endif //!HADES_GRID_HPP
