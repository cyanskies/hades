#ifndef HADES_COLLISION_HPP
#define HADES_COLLISION_HPP

#include <vector>

#include "SFML/Graphics/Rect.hpp"

#include "Hades/Types.hpp"

//provides collision objects and intersection detectors
//NOTE: this isn't a physics class, just geometry for intersection detection
namespace hades
{
	//base collision class, allows collision testing without needing exact types
	class Collider
	{
	public:
		enum class CollideType { RECT, CIRCLE, POINT, MULTIPOINT, NONE };

		Collider() : Type(CollideType::NONE) {}
		Collider(CollideType type) : Type(type) {}
		virtual ~Collider() {}

		CollideType type() const { return Type; }

	protected:
		CollideType Type;
	};

	namespace collision {
		//these return the collision vector, in the same scope as the collision data was sent in.
		//if the size of the rect is 0, then no collision has been detected
		//if only one side of the rect is non-zero, then it is just adjacent
		//if the collision is detected, then the position of the rect, indicates where the 
		//collision starts in the first object(in that objects local space)
		//the size of the rect, shows the total size of collided area
		sf::IntRect test(const Collider &first, const Collider &other);
		sf::IntRect test(const Collider &first, std::vector<const Collider*> other);

		//axis aligned rectangle
		class Rect : public Collider
		{
		public:
			Rect(types::int32 left, types::int32 top, types::int32 width, types::int32 height) : Collider(CollideType::RECT), _rect(left, top, width, height) {}
			Rect(sf::Vector2i position, sf::Vector2i size) : Collider(CollideType::RECT), _rect(position, size) {}

			sf::IntRect getRect() const { return _rect; }

		private:
			sf::IntRect _rect;
		};
		
		class Point : public Collider
		{
		public:
			Point(types::int32 x, types::int32 y) : Collider(CollideType::POINT), _position(x, y) {}
			Point(sf::Vector2i position) : Collider(CollideType::POINT), _position(position) {}

			sf::Vector2i getPostition() const { return _position; }

		private:
			sf::Vector2i _position;
		};

		class MultiPoint : public Collider
		{
		public:
			enum Direction {UP, DOWN, LEFT, RIGHT, ALL};

			MultiPoint(std::vector<sf::Vector2i> points, Direction d) : Collider(CollideType::MULTIPOINT), _points(points),
			_direction(d) {}

			std::vector<sf::Vector2i> getPoints() const { return _points; }
			Direction getDirection() const { return _direction; }

		private:
			Direction _direction;
			std::vector<sf::Vector2i> _points;
		};

		//circular collision type
		class Circle : public Collider
		{
		public:
			Circle(types::int32 x, types::int32 y, types::int32 radius) : Collider(CollideType::CIRCLE), _position(x, y), _radius(radius) {}
			Circle(sf::Vector2i position, types::int32 radius) : Collider(CollideType::CIRCLE), _position(position), _radius(radius) {}

			sf::Vector2i getPosition() const { return _position; }
			types::int32 getRadius() const { return _radius; }

		private:
			sf::Vector2i _position;
			types::int32 _radius;
		};
	}
}

#endif //HADES_COLLISION_HPP
