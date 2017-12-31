#include "Hades/GridArea.hpp"

#include "SFML/Graphics/RenderTarget.hpp"

namespace hades
{
	std::vector<sf::RectangleShape> CreateGrid(const sf::Vector2f &size, const sf::Color &c, types::uint32 cellSize)
	{
		const float thickness = 1.f;
		std::vector<sf::RectangleShape> lines;

		const auto max = std::max(size.x, size.y);

		for (auto i = 0; i <= max; i += cellSize)
		{
			if (i <= size.x)
			{
				auto line = sf::RectangleShape({ thickness, size.y });
				line.setPosition({ static_cast<float>(i), 0.f });
				lines.emplace_back(std::move(line));
			}

			if (i <= size.y)
			{
				auto line = sf::RectangleShape({ size.x, thickness });
				line.setPosition({ 0.f, static_cast<float>(i) });
				lines.emplace_back(std::move(line));
			}
		}

		return lines;
	}

	void GridArea::setSize(const sf::Vector2f &size)
	{
		_size = size;
		_lines = CreateGrid(_size, _colour, _cellSize);
	}

	void GridArea::setColour(const sf::Color &c)
	{
		_colour = c;
		_lines = CreateGrid(_size, _colour, _cellSize);
	}

	void GridArea::setCellSize(types::uint32 s)
	{
		_cellSize = s;
		_lines = CreateGrid(_size, _colour, _cellSize);
	}

	void GridArea::set2dDrawArea(const sf::FloatRect &worldCoords)
	{
		_worldClampRegion = worldCoords;
	}

	void GridArea::draw(sf::RenderTarget &target, sf::RenderStates states) const
	{
		states.transform *= getTransform();

		for (auto &line : _lines)
		{
			auto rect = line.getGlobalBounds();
			states.transform.transformRect(rect);

			//only draw the lines that intersect the 2d camera
			if (rect.intersects(_worldClampRegion))
				target.draw(line, states);
		}
	}	
}