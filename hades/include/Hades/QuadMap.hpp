#ifndef HADES_QUADMAP_HPP
#define HADES_QUADMAP_HPP

#include <vector>

#include "SFML/Graphics/Rect.hpp"

#include "Hades/Types.hpp"

namespace hades
{
	template<class Key>
	struct QuadData;

	template<class Key>
	class QuadNode;

	template<class Key>
	class QuadTree
	{
	public:
		using rect_type = sf::IntRect;
		using key_type = Key;
		using value_type = QuadData<key_type>;
		using node_type = QuadNode<key_type>;

		explicit QuadTree(types::int32 bucket_cap);

		void setAreaSize(const rect_type &area);

		std::vector<value_type> find_collisions(const rect_type &rect) const;

		void insert(const rect_type&, const key_type&);
		void remove(key_type id);

	private:
		node_type _rootNode;
	};
}

#include "Hades/detail/QuadMap.inl"

#endif //HADES_QUADMAP_HPP