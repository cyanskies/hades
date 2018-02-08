#ifndef HADES_GRID_AREA_HPP
#define HADES_GRID_AREA_HPP

#include <vector>

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Transformable.hpp"

#include "Hades/LimitedDraw.hpp"
#include "Hades/Types.hpp"

//Draws a grid with the specified distance between lines within its area

namespace hades
{
	class GridArea : public sf::Drawable, public sf::Transformable, public Camera2dDrawClamp
	{
	public:
		void setSize(const sf::Vector2f &size);
		void setColour(const sf::Color &c);
		void setCellSize(types::uint32 s);

		void limitDrawTo(const sf::FloatRect &worldCoords) override;

		void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

	private:
		sf::Vector2f _size = { 0.f, 0.f };
		sf::Color _colour = sf::Color::White;
		types::uint32 _cellSize = 8u;

		std::vector<sf::RectangleShape> _lines;
		sf::FloatRect _worldClampRegion;
	};
}

#endif //HADES_GRID_AREA_HPP
