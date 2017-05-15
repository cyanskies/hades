namespace hades
{
	template<class Rect, class Key, class Value>
	QuadNode<Rect, Key, Value>::QuadNode(const Rect &map, types::uint8 max_density) : _map(map), _max_density(density)
	{}

	template<class Rect, class Key, class Value>
	std::vector<QuadData<Rect, Key, Value>> QuadNode<Rect, Key, Value>::find_collisions(const Rect &rect) const
	{
		std::vector<const QuadNode<Rect, Key, Value>*> nodes;

		for (auto &c : _children)
		{
			if (rect.intersects(c.getArea()))
				nodes.push_back(&c);
		}

		std::vector<QuadData<Rect, Key, Value>> out;

		for (auto r : _data)
			out.push_back(r);

		for (auto n : nodes)
		{
			auto collisions = n->find_collisions(rect);
			out.insert(out.end(), collisions.begin(), collisions.end());
		}

		return out;
	}

	template<class Rect, class Key, class Value>
	void QuadNode<Rect, Key, Value>::insert(const QuadData<Rect, Key, Value> &data)
	{
		//no chilren, insert into this node
		if (_children.empty() && _data.size() < _max_density)
		{
			_data.push_back(data);
			return;
		}

		//no children, split this node
		if (_children.empty() && _data.size() >= _max_density)
		{
			//create four chilren, then reinsert the current entities held in data.
			//then insert this entity
			int halfwidth = (_map.width / 2) + 1, halfheight = (_map.height / 2) + 1;
			Rect tlrect(_map.left, _map.top, halfwidth, halfheight), trrect(_map.left + halfwidth, _map.top, halfwidth, halfheight),
				blrect(_map.left, _map.top + halfheight, halfwidth, halfheight), brrect(_map.left + halfwidth, _map.top + halfheight, halfwidth, halfheight);

			_children.insert(QuadNode(tlrect));
			_children.insert(QuadNode(trrect));
			_children.insert(QuadNode(blrect));
			_children.insert(QuadNode(brrect));

			auto rects = _data;
			_data.clear();

			for (auto ent : rects)
			{
				insert({ ent.id, ent.layer, ent.collideRect });
			}
		}

		//has children, insert into relevent child node
		std::vector<std::pair<Rect, QuadNode*> > nodes;
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

	template<class Rect, class Key, class Value>
	void QuadNode<Rect, Key, Value>::remove(Key id)
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
			_data.erase(std::remove_if(_data.begin(), _data.end(), [id](QuadData<Rect, Key, Value> &in) { return in.id == id; }), _data.end());

		//TODO: rotate structure if we fall bellow parents MAXDENSITY
		// if _data.size and _stored.size < max_density
		// gobble children and store them all in data
	}

	template<class Rect, class Key, class Value>
	void QuadMap<Rect, Key, Value>::setAreaSize(const Rect &map)
	{
		auto children = _rootNode.find_collisions(_rootNode.getArea());
		auto density = std::min(map.width / 2, map.height / 2);

		_rootNode = QuadNode<Rect, Key, Value>(map, density);

		for (auto &c : children)
			_rootNode.insert(c);
	}

	template<class Rect, class Key, class Value>
	std::vector<QuadData<Rect, Key, Value>> QuadMap<Rect, Key, Value>::find_collisions(const Rect &rect) const
	{
		return _rootNode.find_collisions(rect);
	}

	template<class Rect, class Key, class Value>
	void QuadMap<Rect, Key, Value>::insert(Rect r, Key k, Value v)
	{
		_rootNode.remove(k);
		_rootNode.insert({r, k, v});
	}

	template<class Rect, class Key, class Value>
	void QuadMap<Rect, Key, Value>::remove(Key id)
	{
		_rootNode.remove(id);
	}
}