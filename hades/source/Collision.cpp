#include "Hades/Collision.hpp"

#include <algorithm>
#include <cassert>

#include "Hades/Types.hpp"
#include "Hades/vector_math.hpp"

namespace hades
{
	namespace collision
	{
		//reverse the params if they don't match a cmoparison combination
		template <class T, class U>
		sf::IntRect collisionTest(const T &lhs, const U &rhs)
		{
			assert(false && "don't call this again");
			return sf::IntRect();
		}

		//test against themselves
		template<>
		sf::IntRect collisionTest<Rect, Rect>(const Rect &lhs, const Rect &rhs)
		{
			sf::IntRect intersection;
			lhs.getRect().intersects(rhs.getRect(), intersection);
			return intersection;
		}

		template<>
		sf::IntRect collisionTest<Point, Point>(const Point &lhs, const Point &rhs)
		{
			sf::IntRect rect;

			if (lhs.getPosition() == rhs.getPosition())
			{
				rect.width = 1;
				rect.height = 1;
			}

			return rect;
		}

		template<>
		sf::IntRect collisionTest<MultiPoint, MultiPoint>(const MultiPoint &lhs, const MultiPoint &rhs)
		{
			//rect position is the top leftmost point to collide
			//rect size is the distance to the farthest one to collide
			return sf::IntRect();
		}

		template<>
		sf::IntRect collisionTest<Circle, Circle>(const Circle &lhs, const Circle &rhs)
		{
			auto vector = static_cast<sf::Vector2f>(rhs.getPosition() - lhs.getPosition());
			auto distance = vector::length(vector.x, vector.y);
			auto range = distance - (lhs.getRadius() + rhs.getRadius());
			if (range < 0.f)
			{
				float x, y;
				std::tie(x, y) = vector::resize(vector.x, vector.y, range);
				return sf::IntRect(static_cast<int>(x), static_cast<int>(y),
					static_cast<int>(distance), static_cast<int>(distance));
			}

			return sf::IntRect();
		}

		//test all the unique combinations
		template<>
		sf::IntRect collisionTest<Rect, Point>(const Rect &lhs, const Point &rhs)
		{
			sf::IntRect rect;
			if (lhs.getRect().contains(rhs.getPosition()))
			{
				rect = lhs.getRect();
				auto right = rhs.getPosition();

				rect.top = rect.top - right.y;
				rect.left = rect.left - right.x;
				rect.width = 1;
				rect.height = 1;
			}

			return rect;
		}

		template<>
		sf::IntRect collisionTest<Rect, MultiPoint>(const Rect &lhs, const MultiPoint &rhs)
		{
			return sf::IntRect();
		}

		template<>
		sf::IntRect collisionTest<Rect, Circle>(const Rect &lhs, const Circle &rhs)
		{
			return sf::IntRect();
		}

		template<>
		sf::IntRect collisionTest<Point, Rect>(const Point &lhs, const Rect &rhs)
		{
			auto p = lhs.getPosition();
			sf::IntRect rect(p.x, p.y, 0, 0);
			if (rhs.getRect().contains(p))
			{
				rect.width = 1;
				rect.height = 1;
			}

			return rect;
		}

		template<>
		sf::IntRect collisionTest<Point, MultiPoint>(const Point &lhs, const MultiPoint &rhs)
		{
			return sf::IntRect();
		}

		template<>
		sf::IntRect collisionTest<Point, Circle>(const Point &lhs, const Circle &rhs)
		{
			return sf::IntRect();
		}

		template<>
		sf::IntRect collisionTest<MultiPoint, Rect>(const MultiPoint &lhs, const Rect &rhs)
		{
			return sf::IntRect();
		}

		template<>
		sf::IntRect collisionTest<MultiPoint, Point>(const MultiPoint &lhs, const Point &rhs)
		{
			return sf::IntRect();
		}

		template<>
		sf::IntRect collisionTest<MultiPoint, Circle>(const MultiPoint &lhs, const Circle &rhs)
		{
			return sf::IntRect();
		}

		template<>
		sf::IntRect collisionTest<Circle, Rect>(const Circle &lhs, const Rect &rhs)
		{
			return sf::IntRect();
		}

		template<>
		sf::IntRect collisionTest<Circle, Point>(const Circle &lhs, const Point &rhs)
		{
			auto dist = rhs.getPosition() - lhs.getPosition();
			sf::IntRect rect;
			auto distf = static_cast<sf::Vector2f>(dist);
			if (vector::length(distf.x, distf.y) < lhs.getRadius())
			{
				rect.left = dist.x;
				rect.top = dist.y;
				rect.width = 1;
				rect.height = 1;
			}

			return rect;
		}

		template<>
		sf::IntRect collisionTest<Circle, MultiPoint>(const Circle &lhs, const MultiPoint &rhs)
		{
			return sf::IntRect();
		}

		template<typename T>
		sf::IntRect secondTest(const T *first, const Collider &other)
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

		sf::IntRect test(const Collider &first, const Collider &other)
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

		sf::IntRect test(const Collider &first, std::vector<const Collider*> other)
		{
			sf::IntRect collisionvect;

			for (auto c : other)
			{
				auto collide = test(first, *c);

				//TODO: return both position of intersect, and intersection area
				//merge collide with collisionRect
				if (std::abs(collisionvect.left) < std::abs(collide.left))
					collisionvect.left = collide.left;

				if (std::abs(collisionvect.top) < std::abs(collide.top))
					collisionvect.top = collide.top;
			}

			return collisionvect;
		}
	}
}
