#include <cassert>
#include <map>
#include <vector>

#include "Hades/exceptions.hpp"
#include "Hades/QuadMap.hpp"

namespace hades
{
	template<class Key>
	struct QuadData
	{
		using key_type = Key;
		using rect_type = sf::IntRect;

		key_type key;
		rect_type rect;
	};

	template<class Rect>
	constexpr Rect MakeMaxRect()
	{
		using T = decltype(Rect::left);
		auto min = std::numeric_limits<T>::min();
		auto max = std::numeric_limits<T>::max();
		return Rect(min, min, max, max);
	}

	template<class Key>
	class QuadNode
	{
	public:
		using rect_type = sf::IntRect;
		using key_type = Key;
		using value_type = QuadData<key_type>;
		using node_type = QuadNode<key_type>;

		QuadNode(const rect_type &area, types::uint32 _bucket_cap) : _area(area), _bucket_cap(_bucket_cap)
		{
			_children.reserve(4);
		}

		rect_type getArea() const
		{
			return _area;
		}

		std::vector<value_type> find_collisions(const rect_type &rect) const
		{
			std::vector<const node_type*> nodes;

			for (auto &c : _children)
			{
				if (rect.intersects(c.getArea()))
					nodes.push_back(&c);
			}

			std::vector<value_type> out;

			for (auto r : _data)
				out.push_back(r);

			for (auto n : nodes)
			{
				auto collisions = n->find_collisions(rect);
				out.insert(out.end(), collisions.begin(), collisions.end());
			}

			return out;
		}

		void insert(const value_type &data)
		{
			//no chilren, insert into this node
			if (_children.empty() && _data.size() <= _bucket_cap)
			{
				_stored.insert(std::make_pair(data.key, this));
				_data.push_back(data);
				return;
			}

			//no children, split this node
			if (_children.empty() && _data.size() > _bucket_cap)
			{
				//create four chilren, then reinsert the current entities held in data.
				//then insert this entity
				int halfwidth = (_area.width / 2) + 1, halfheight = (_area.height / 2) + 1;
				rect_type tlrect(_area.left, _area.top, halfwidth, halfheight), trrect(tlrect.left + tlrect.width, tlrect.top, halfwidth, halfheight),
					blrect(tlrect.left, tlrect.top + tlrect.height, halfwidth, halfheight), brrect(tlrect.left + tlrect.width, tlrect.top + tlrect.height, halfwidth, halfheight);

				_children.push_back(node_type(tlrect, _bucket_cap));
				_children.push_back(node_type(trrect, _bucket_cap));
				_children.push_back(node_type(blrect, _bucket_cap));
				_children.push_back(node_type(brrect, _bucket_cap));

				assert(_children.size() == 4);

				auto rects = _data;
				_data.clear();
				_stored.clear();

				for (auto r : rects)
					insert(r);
			}

			//has children, insert into relevent child node
			std::vector<node_type*> nodes;
			for (auto &c : _children)
			{
				if (data.rect.intersects(c.getArea()))
					nodes.push_back(&c);
			}

			//insert this rect into target child and leave breadcrumbs pointing to it
			if (nodes.size() == 1)
			{
				_stored.insert(std::make_pair(data.key, nodes.front()));
				nodes.front()->insert(data);
			}
			//keep the rect in this node if it intersects multiple child nodes.
			else
			{
				_stored.insert(std::make_pair(data.key, this));
				_data.push_back(data);
			}
		}

		void remove(key_type id)
		{
			//remove if from children, if present
			auto s = _stored.find(id);
			if (s != _stored.end())
			{
				if (s->second == this)
				{
					//remove entity from _data
					_data.erase(std::remove_if(_data.begin(), _data.end(),
						[&id](const value_type &in) { return in.key == id; }), _data.end());
				}
				else
				{
					assert(s->first == id);
					s->second->remove(id);
					_stored.erase(s);
					return;
				}
			}
		}

	private:
		rect_type _area = MakeMaxRect<rect_type>();
		types::uint32 _bucket_cap = 1u;
		std::vector<value_type> _data;
		std::map<key_type, node_type*> _stored;
		using _child_vector_type = std::vector<node_type>;
		_child_vector_type _children;
	};

	template<class Key>
	QuadTree<Key>::QuadTree(const rect_type &area, types::int32 _bucket_cap) : _rootNode(area, static_cast<types::uint32>(_bucket_cap))
	{
		if (_bucket_cap < 1)
			throw invalid_argument("QuadTree bucket capacity must be greater than 0");
	}

	template<class Key>
	std::vector<typename QuadTree<Key>::value_type> QuadTree<Key>::find_collisions(const rect_type &rect) const
	{
		return _rootNode.find_collisions(rect);
	}

	template<class Key>
	void QuadTree<Key>::insert(const rect_type &r, const key_type &k)
	{
		_rootNode.remove(k);
		_rootNode.insert({k, r});
	}

	template<class Key>
	void QuadTree<Key>::remove(key_type id)
	{
		_rootNode.remove(id);
	}
}
