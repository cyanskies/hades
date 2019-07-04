#include "hades/quad_map.hpp"

#include <cassert>
#include <map>
#include <limits>
#include <stack>
#include <vector>

#include "hades/exceptions.hpp"

namespace hades
{
	template<class Key, typename Rect>
	struct quad_data
	{
		using key_type = Key;
		using rect_type = Rect;

		key_type key;
		rect_type rect;
	};

	template<typename T, typename U>
	constexpr bool operator==(const quad_data<T, U>&l, const quad_data<T, U>& r) noexcept
	{
		return l.key == r.key &&
			l.rect == r.rect;
	}

	template<typename T, typename U>
	constexpr bool operator<(const quad_data<T, U>& l, const quad_data<T, U>& r) noexcept
	{
		return std::tie(l.key, l.rect) < std::tie(r.key, r.rect);
	}

	template<template<typename> typename Rect, typename T>
	constexpr Rect<T> make_max_rect()
	{
		auto min = std::numeric_limits<T>::min();
		auto max = std::numeric_limits<T>::max();
		return Rect<T>(min, min, max, max);
	}

	template<class Key, typename Rect>
	class quad_node
	{
	public:
		using rect_type = Rect;
		using key_type = Key;
		using value_type = quad_data<key_type, rect_type>;
		using node_type = quad_node<key_type, rect_type>;

		quad_node(const rect_type &area, types::uint32 _bucket_cap) : _area(area), _bucket_cap(_bucket_cap)
		{
			_children.reserve(4);
		}

		rect_type area() const
		{
			return _area;
		}

		std::vector<value_type> find_collisions(const rect_type &rect) const
		{
			std::vector<const node_type*> nodes;

			for (auto &c : _children)
			{
				if (collision_test(rect, c.area()))
					nodes.push_back(&c);
			}

			std::vector<value_type> out;

			for (auto r : _data)
				out.push_back(r);

			for (auto n : nodes)
			{
				auto collisions = n->find_collisions(rect);
				std::move(std::begin(collisions), std::end(collisions), std::back_inserter(out));
				//out.insert(out.end(), collisions.begin(), collisions.end());
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
				const auto halfwidth = (_area.width / 2) + 1, halfheight = (_area.height / 2) + 1;
				const rect_type tlrect{ _area.x, _area.y, halfwidth, halfheight },
					trrect{ tlrect.x + tlrect.width, tlrect.y, halfwidth, halfheight },
					blrect{ tlrect.x, tlrect.y + tlrect.height, halfwidth, halfheight },
					brrect{ tlrect.x + tlrect.width, tlrect.y + tlrect.height, halfwidth, halfheight };

				_children.push_back(node_type(tlrect, _bucket_cap));
				_children.push_back(node_type(trrect, _bucket_cap));
				_children.push_back(node_type(blrect, _bucket_cap));
				_children.push_back(node_type(brrect, _bucket_cap));

				assert(_children.size() == 4);

				const auto rects = _data;
				_data.clear();
				_stored.clear();

				for (auto r : rects)
					insert(r);
			}

			//has children, insert into relevent child node
			std::vector<node_type*> nodes;
			for (auto &c : _children)
			{
				if (collision_test(data.rect, c.area()))
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

		template<typename T, typename U>
		friend constexpr bool operator==(const quad_tree<T, U>& lhs, const quad_tree<T, U>& rhs);
		template<typename T, typename U>
		friend std::vector<typename quad_node<T, U>::value_type> flatten(const quad_node<T, U>& n);

	private:
		rect_type _area = make_max_rect<rect_type>();
		types::uint32 _bucket_cap = 1u;
		std::vector<value_type> _data;
		std::map<key_type, node_type*> _stored;
		using _child_vector_type = std::vector<node_type>;
		_child_vector_type _children;
	};

	template<typename T, typename U>
	std::vector<typename quad_node<T, U>::value_type> flatten(const quad_node<T, U>& n)
	{
		auto out = std::vector<typename quad_node<T, U>::value_type>{};
		std::stack<const quad_node<T, U>*> stack;
		stack.emplace(&n);
		while (!std::empty(stack))
		{
			const auto top = stack.top();
			stack.pop();
			std::copy(std::begin(top->_data), std::end(top->_data), std::back_inserter(out));
			for (const auto& c : top->_children)
				stack.push(&c);
		}
		return out;
	}

	template<typename T, typename U>
	constexpr bool operator==(const quad_tree<T, U>& lhs, const quad_tree<T, U>& rhs)
	{
		if (lhs._rootNode._area != rhs._rootNode._area)
			return false;

		auto lnode = flatten(lhs._rootNode);
		auto rnode = flatten(rhs._rootNode);

		if (std::size(lnode) != std::size(rnode))
			return false;

		//sort the nodes
		std::sort(std::begin(lnode), std::end(lnode));
		std::sort(std::begin(rnode), std::end(rnode));

		return lnode == rnode;
	}

	template<class Key, typename Rect>
	quad_tree<Key, Rect>::quad_tree(const rect_type &area, types::int32 _bucket_cap) : _rootNode(area, static_cast<types::uint32>(_bucket_cap))
	{
		if (_bucket_cap < 1)
			throw invalid_argument("quad_tree bucket capacity must be greater than 0");
	}

	template<class Key, typename Rect>
	std::vector<typename quad_tree<Key, Rect>::value_type> quad_tree<Key, Rect>::find_collisions(const rect_type &rect) const
	{
		return _rootNode.find_collisions(rect);
	}

	template<class Key, typename Rect>
	void quad_tree<Key, Rect>::insert(const rect_type &r, const key_type &k)
	{
		_rootNode.remove(k);
		_rootNode.insert({k, r});
	}

	template<class Key, typename Rect>
	void quad_tree<Key, Rect>::remove(key_type id)
	{
		_rootNode.remove(id);
	}
}
