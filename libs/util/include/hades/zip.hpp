#ifndef HADES_UTIL_ZIP_HPP
#define HADES_UTIL_ZIP_HPP

#include "detail/zip.inl"

//zip functions can be used to iterate over multiple ranges
//eg.
// for container<int> 'a' and container<char> 'b'
// for(auto [i, c] : hades::zip(a, b))
//
// can also be used to generate iter pairs
// for other loopstyles
// begin = zip(begin(a), begin(b));
// end = zip(end(a), end(b));

// generated zip iterators are never random access, and don't qualify for
// use in sort and many other algorithms
// the iterator category is the lowest common category of the provided integers
// but is never greater than bidirectional iterator

namespace hades
{
	template<typename ...Iterators, std::enable_if_t<(detail::is_iterator_v<Iterators> && ...), int> = 0>
	detail::zip_iterator<Iterators...> zip(Iterators ...iters) noexcept
	{
		return detail::zip_iterator{ iters... };
	}

	template<typename ...Containers, std::enable_if_t<(detail::is_container_v<Containers> && ...), int> = 0>
	detail::zip_container_adapter<Containers...> zip(Containers& ...containers) noexcept
	{
		return detail::zip_container_adapter{ containers... };
	}
}

#endif // !HADES_UTIL_ZIP_HPP
