#ifndef HADES_QUADMAP_HPP
#define HADES_QUADMAP_HPP

#include <vector>

#include "hades/collision.hpp" //for rect->rect collision test
#include "hades/types.hpp"

namespace hades
{
	template<class Key, typename Rect>
	struct quad_data;

	template<class Key, typename Rect>
	class quad_node;

	template<class Key, typename Rect = rect_t<types::int32>>
	class quad_tree
	{
	public:
		using rect_type = Rect;
		using key_type = Key;
		using value_type = quad_data<key_type, rect_type>;
		using node_type = quad_node<key_type, rect_type>;

		quad_tree() : _rootNode(rect_type{}, 1) {}
		quad_tree(const rect_type &area, types::int32 bucket_cap);

		template<typename T, typename U>
		friend constexpr bool operator==(const quad_tree<T, U>&, const quad_tree<T, U>&);

		std::vector<value_type> find_collisions(const rect_type &rect) const;

		void insert(const rect_type&, const key_type&);
		void remove(key_type id);

	private:
		node_type _rootNode;
	};
}

#include "hades/detail/quad_map.inl"

#endif //HADES_QUADMAP_HPP
