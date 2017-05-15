#ifndef HADES_QUADMAP_HPP
#define HADES_QUADMAP_HPP

#include <map>
#include <vector>

#include "Hades/Types.hpp"

namespace hades
{
	template<class Rect, class Key, class Value>
	struct QuadData
	{
		using rect_type = Rect;
		using key_type = Key;
		using stored_type = Value;

		Key key;
		Value value;
		Rect rect;
	};

	template<class Rect, class Key, class Value>
	class QuadNode
	{
	public:
		using rect_type = Rect;
		using key_type = Key;
		using stored_type = Value;
		using value_type = QuadData<rect_type, key_type, value_type>;

		QuadNode() {}

		explicit QuadNode(const rect_type &area, types::uint8 max_density);

		void setMapSize(const rect_type &area);
		rect_type getArea() const;

		std::vector<value_type> find_collisions(const rect_type &rect) const;

		void insert(const value_type &data);
		void remove(key_type id);

	private:
		rect_type _area;
		types::uint8 _max_density;

		using _child_vector_type = std::vector<QuadNode<rect_type, key_type, stored_type>>;

		std::vector<value_type> _data;
		std::map<key_type, QuadNode<Rect, Key, Value>*> _stored;
		_child_vector_type _children;
	};

	template<class Rect, class Key,  class Value>
	class QuadMap
	{
	public:
		using rect_type = Rect;
		using key_type = Key;
		using stored_type = Value;
		using value_type = QuadData<rect_type, key_type, stored_type>;
		using node_type = QuadNode<rect_type, key_type, value_type>;

		void setAreaSize(const rect_type &area);

		// This returns possible collisions, by generating a list of nearby rects. You still need to check for actual intersections manually.
		std::vector<value_type> find_collisions(const rect_type &rect) const;

		void insert(const key_type, const value_type, const rect_type);
		void remove(int id);

	private:
		node_type _rootNode;
	};
}

#include "Hades/detail/QuadMap.inl"

#endif //HADES_QUADMAP_HPP