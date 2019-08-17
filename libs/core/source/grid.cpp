#include "hades/grid.hpp"

#include <array>
#include <cassert>

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/animation.hpp"

namespace hades
{
	static poly_quad make_quad(vector_float p, vector_float s, colour c)
	{
		return make_quad_colour({ p.x, p.y, s.x, s.y }, c);
	}

	static std::vector<sf::Vertex> make_grid(const grid::grid_properties &p)
	{
		const auto rows = static_cast<int32>(std::floor(p.grid_size.y / p.cell_size));
		const auto cols = static_cast<int32>(std::floor(p.grid_size.x / p.cell_size));

		std::vector<sf::Vertex> verts{};
		verts.reserve(rows * cols);

		//make rows
		constexpr auto row_left = 0.f;
		const auto row_right = p.grid_size.x;

		for (auto y = 0.f; y < p.grid_size.y; y += p.cell_size)
		{
			auto quad = make_quad({ row_left, y },
				{ p.grid_size.x, p.line_thickness }, p.line_colour);
			
			std::move(std::begin(quad), std::end(quad), std::back_inserter(verts));
		}

		//make coloumns
		constexpr auto coloumn_top = 0.f;
		const auto coloumn_bottom = p.grid_size.y;

		for (auto x = 0.f; x < p.grid_size.x; x += p.cell_size)
		{
			auto quad = make_quad({ x, coloumn_top },
				{ p.line_thickness, p.grid_size.y }, p.line_colour);

			std::move(std::begin(quad), std::end(quad), std::back_inserter(verts));
		}
		
		assert(verts.size() % 3 == 0);
		return verts;
	}

	void grid::set_size(vector_float s)
	{
		_properties.grid_size = s;
		_verticies = std::move(make_grid(_properties));
	}

	void grid::set_cell_size(float s)
	{
		_properties.cell_size = s;
		_verticies = std::move(make_grid(_properties));
	}

	void grid::set_line_thickness(float t)
	{
		_properties.line_thickness = t;
		_verticies = std::move(make_grid(_properties));
	}

	void grid::set_colour(colour c)
	{
		_properties.line_colour = c;
		_verticies = std::move(make_grid(_properties));
	}

	void grid::set_all(grid_properties p)
	{
		_properties = std::move(p);
		_verticies = make_grid(_properties);
	}

	void grid::draw(sf::RenderTarget &t, const sf::RenderStates s) const
	{
		t.draw(_verticies.data(), _verticies.size(),
			sf::PrimitiveType::Triangles, s);
	}
}