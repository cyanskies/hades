#ifndef HADES_GRID_HPP
#define HADES_GRID_HPP

#include <vector>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/colour.hpp"
#include "hades/math.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	class grid : public sf::Drawable
	{
	public:
		struct grid_properties
		{
			vector_float grid_size{};
			float cell_size{};
			float line_thickness{ 1.f };
			colour line_colour{};
		};

		void set_size(vector_float);
		void set_cell_size(float);
		void set_line_thickness(float);
		void set_colour(colour);
		void set_all(grid_properties);

		void draw(sf::RenderTarget&, sf::RenderStates = sf::RenderStates{}) const override;

	private:
		grid_properties _properties;
		std::vector<sf::Vertex> _verticies;
	};
}

#endif //!HADES_GRID_HPP
