#include "Hades/Collision.hpp"

#include <algorithm>
#include <cassert>

namespace hades
{
	namespace collision
	{
		//reverse the params if they don't match a cmoparison combination
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

		//test all the unique combinations
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

		template<typename T>
		sf::Vector2i secondTest(const T *first, const Collider &other)
		{
			//second test tests the second parameter of test
			//now we can dispatch to the correct function above
			if (other.type() == Collider::CollideType::RECT)
				return collisionTest(*first, static_cast<const Rect&>(other));
			else if (other.type() == Collider::CollideType::POINT)
				return collisionTest(*first, static_cast<const Point&>(other));
			else if (other.type() == Collider::CollideType::MULTIPOINT)
				return collisionTest(*first, static_cast<const MultiPoint&>(other));
			else
				return collisionTest(*first, static_cast<const Circle&>(other));
		}

		sf::Vector2i test(const Collider &first, const Collider &other)
		{
			assert(first.type() != Collider::CollideType::NONE);
			assert(other.type() != Collider::CollideType::NONE);
			//test collider types, then choose the correct algirithm

			if (first.type() == Collider::CollideType::RECT)
				return secondTest(static_cast<const Rect*>(&first), other);
			else if(first.type() == Collider::CollideType::POINT)
				return secondTest(static_cast<const Point*>(&first), other);
			else if(first.type() == Collider::CollideType::MULTIPOINT)
				return secondTest(static_cast<const MultiPoint*>(&first), other);
			else
				return secondTest(static_cast<const Circle*>(&first), other);
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