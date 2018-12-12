#include "hades/grid.hpp"

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/RenderTarget.hpp"

namespace hades
{
	static constexpr auto quad_vert_count = 6;
	static std::array<sf::Vertex, quad_vert_count> make_quad(vector_float p, vector_float s, sf::Color c)
	{
		return std::array<sf::Vertex, quad_vert_count>{ {
				//first tri
			{ {p.x, p.y}, c },
			{ {p.x + s.x, p.y}, c },
			{ {p.x, p.y + s.y}, c },
			//second tri
			{ {p.x + s.x, p.y}, c },
			{ {p.x + s.x, p.y + s.y}, c },
			{ {p.x, p.y + s.y}, c }
		}};
	}

	static std::vector<sf::Vertex> make_grid(const grid::grid_properties &p)
	{
		const auto sf_colour = sf::Color{ p.line_colour.r, p.line_colour.g, p.line_colour.b, p.line_colour.a };

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
				{ p.grid_size.x, p.line_thickness }, sf_colour);
			
			std::move(std::begin(quad), std::end(quad), std::back_inserter(verts));
		}

		//make coloumns
		constexpr auto coloumn_top = 0.f;
		const auto coloumn_bottom = p.grid_size.y;

		for (auto x = 0.f; x < p.grid_size.x; x += p.cell_size)
		{
			auto quad = make_quad({ x, coloumn_top },
				{ p.line_thickness, p.grid_size.y }, sf_colour);

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