#ifndef HADES_COLLISION_HPP
#define HADES_COLLISION_HPP

#include <vector>

#include "SFML/Graphics/Rect.hpp"

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
		sf::Vector2i test(const Collider &first, const Collider &other);
		sf::Vector2i test(const Collider &first, std::vector<const Collider*> other);

		//axis aligned rectangle
		class Rect : public Collider
		{
		public:
			Rect(int left, int top, int width, int height) : Collider(CollideType::RECT), _rect(left, top, width, height) {}
			Rect(sf::Vector2i position, sf::Vector2i size) : Collider(CollideType::RECT), _rect(position, size) {}

			sf::IntRect getRect() const { return _rect; }

		private:
			sf::IntRect _rect;
		};
		
		class Point : public Collider
		{
		public:
			Point(int x, int y) : Collider(CollideType::POINT), _position(x, y) {}
			Point(sf::Vector2i position) : Collider(CollideType::POINT), _position(position) {}

			sf::Vector2i getPostition() const { return _position; }

		private:
			sf::Vector2i _position;
		};

		class MultiPoint : public Collider
		{
		public:
			MultiPoint(std::vector<sf::Vector2i> points) : Collider(CollideType::MULTIPOINT), _points(points) {}

			std::vector<sf::Vector2i> getPoints() const { return _points; }

		private:
			std::vector<sf::Vector2i> _points;
		};

		//circular collision type
		class Circle : public Collider
		{
		public:
			Circle(int x, int y, int radius) : Collider(CollideType::CIRCLE), _position(x, y), _radius(radius) {}
			Circle(sf::Vector2i position, int radius) : Collider(CollideType::CIRCLE), _position(position), _radius(radius) {}

			int getRadius() const { return _radius; }

		private:
			sf::Vector2i _position;
			int _radius;
		};
	}
}

#endif //HADES_COLLISION_HPP
