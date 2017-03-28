#include "Hades/Collision.hpp"

#include <algorithm>
#include <cassert>

namespace hades
{
	namespace collision
	{
		//rect
		template <class T, class U>
		sf::Vector2i collisionTest(const T &lhs, const U &rhs)
		{
			return collisionTest(rhs, lhs);
		}

		//test against themselves
		template<>
		sf::Vector2i collisionTest<Rect, Rect>(const Rect &lhs, const Rect &rhs)
		{
			return sf::Vector2i();
		}
		
		template<>
		sf::Vector2i collisionTest<Point, Point>(const Point &lhs, const Point &rhs)
		{
			return sf::Vector2i();
		}

		template<>
		sf::Vector2i collisionTest<MultiPoint, MultiPoint>(const MultiPoint &lhs, const MultiPoint &rhs)
		{
			return sf::Vector2i();
		}

		template<>
		sf::Vector2i collisionTest<Circle, Circle>(const Circle &lhs, const Circle &rhs)
		{
			return sf::Vector2i();
		}

		template<>
		sf::Vector2i collisionTest<Rect, Point>(const Rect &lhs, const Point &rhs)
		{
			return sf::Vector2i();
		}

		template<>
		sf::Vector2i collisionTest<Rect, MultiPoint>(const Rect &lhs, const MultiPoint &rhs)
		{
			return sf::Vector2i();
		}

		template<>
		sf::Vector2i collisionTest<Rect, Circle>(const Rect &lhs, const Circle &rhs)
		{
			return sf::Vector2i();
		}

		template<>
		sf::Vector2i collisionTest<Point, MultiPoint>(const Point &lhs, const MultiPoint &rhs)
		{
			return sf::Vector2i();
		}

		template<>
		sf::Vector2i collisionTest<Point, Circle>(const Point &lhs, const Circle &rhs)
		{
			return sf::Vector2i();
		}

		template<>
		sf::Vector2i collisionTest<MultiPoint, Circle>(const MultiPoint &lhs, const Circle &rhs)
		{
			return sf::Vector2i();
		}

		sf::Vector2i test(const Collider &first, const Collider &other)
		{
			assert(first.type() != Collider::CollideType::NONE);
			assert(other.type() != Collider::CollideType::NONE);
			//test collider types, then choose the correct algirithm

			return sf::Vector2i();
		}

		sf::Vector2i test(const Collider &first, std::vector<const Collider*> other)
		{
			sf::Vector2i collisionvect;

			for (auto c : other)
			{
				auto collide = test(first, *c);

				//merge collide with collisionRect
				if (std::abs(collisionvect.x) < std::abs(collide.x))
					collisionvect.x = collide.x;

				if (std::abs(collisionvect.y) < std::abs(collide.y))
					collisionvect.y = collide.y;
			}

			return collisionvect;
		}
	}
}