#ifndef HADES_GRID_HPP
#define HADES_GRID_HPP

#include <vector>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RenderStates.hpp"

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

		grid() = default;

		grid(grid_properties p)
		{
			set_all(p);
			return;
		}

		void set_size(vector_float);
		void set_cell_size(float);
		void set_line_thickness(float);
		void set_colour(colour);
		void set_all(grid_properties);

	protected:
		void draw(sf::RenderTarget&, const sf::RenderStates& = sf::RenderStates{}) const override;

	private:
		grid_properties _properties;
		quad_buffer _quads{ sf::VertexBuffer::Usage::Static };
	};
}

#endif //!HADES_GRID_HPP
