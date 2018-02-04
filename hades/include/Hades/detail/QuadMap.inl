#include <map>
#include <vector>

#include "Hades/QuadMap.hpp"

namespace hades
{
	template<class Key>
	struct QuadData
	{
		using key_type = Key;

		key_type key;
		sf::IntRect rect;
	};

	template<class Key>
	class QuadNode
	{
	public:
		using rect_type = sf::IntRect;
		using key_type = Key;
		using value_type = QuadData<key_type>;
		using node_type = QuadNode<key_type>;

		QuadNode() = default;

		explicit QuadNode(const rect_type &area, types::int32 _bucket_cap) : _area(map), _bucket_cap(max_density)
		{}

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
				_data.push_back(data);
				return;
			}

			//no children, split this node
			if (_children.empty() && _data.size() > _bucket_cap)
			{
				//create four chilren, then reinsert the current entities held in data.
				//then insert this entity
				int halfwidth = (_area.width / 2) + 1, halfheight = (_area.height / 2) + 1;
				Rect tlrect(_area.left, _area.top, halfwidth, halfheight), trrect(_area.left + halfwidth, _area.top, halfwidth, halfheight),
					blrect(_area.left, _area.top + halfheight, halfwidth, halfheight), brrect(_area.left + halfwidth, _area.top + halfheight, halfwidth, halfheight);

				_children.insert(node_type(tlrect));
				_children.insert(node_type(trrect));
				_children.insert(node_type(blrect));
				_children.insert(node_type(brrect));

				auto rects = _data;
				_data.clear();

				for (auto r : rects)
					insert(r);
			}

			//has children, insert into relevent child node
			std::vector<std::pair<rect_type, node_type*> > nodes;
			for (auto &c : _children)
			{
				if (data.rect.intersects(c.getArea()))
				{
					auto node = std::make_pair(c.getArea(), &c);
					nodes.push_back(node);
				}
			}

			//insert this rect into target child and leave breadcrumbs pointing to it
			if (nodes.size() == 1)
			{
				_stored.insert(std::make_pair(data.id, nodes.front().second));
				nodes.front().second->insert(data);
			}
			//keep the rect in this node if it intersects multiple child nodes.
			else if (nodes.size() > 1)
			{
				_data.push_back(data);
			}
		}

		void remove(key_type id)
		{
			//remove if from children, if present
			auto s = _stored.find(id);
			if (s != _stored.end())
			{
				assert(s.first = id);
				s.second->remove(id);
				_stored.erase(s);
				return;
			}

			//remove entity from _data
			if (!_data.empty())
				_data.erase(std::remove_if(_data.begin(), _data.end(), [&id](const value_type &in) { return in.id == id; }), _data.end());

			//TODO: rotate structure if we fall bellow parents MAXDENSITY
			// if _data.size and _stored.size < max_density
			// gobble children and store them all in data
		}

	private:
		rect_type _area;
		types::int32 _bucket_cap;
		std::vector<value_type> _data;
		std::map<key_type, node_type*> _stored;
		using _child_vector_type = std::vector<node_type>;
		_child_vector_type _children;
	};

	template<class Key>
	void QuadTree<Key>::setAreaSize(const rect_type &map)
	{
		auto children = _rootNode.find_collisions(_rootNode.getArea());
		auto density = std::min(map.width / 2, map.height / 2);

		_rootNode = node_type(map, density);

		for (auto &c : children)
			_rootNode.insert(c);
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
		_rootNode.insert({r, k});
	}

	template<class Key>
	void QuadTree<Key>::remove(key_type id)
	{
		_rootNode.remove(id);
	}
}
