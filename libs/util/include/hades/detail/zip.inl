#include <iterator>
#include <tuple>
#include <type_traits>

namespace hades::detail
{
	// is iterator type trait
	template<typename T, typename = void>
	struct is_iterator : std::false_type {};
	template<typename T>
	struct is_iterator <T, std::void_t<typename std::iterator_traits<T>::iterator_category>> : std::true_type {};
	template<typename T>
	constexpr auto is_iterator_v = is_iterator<T>::value;

	// is container type trait
	template <typename T, typename = void>
	struct is_container : std::false_type {};
	template <typename T>
	struct is_container<T, std::void_t<decltype(std::declval<T>().begin()),
		decltype(std::declval<T>().end())>>
		: std::true_type {};
	template<typename T>
	constexpr auto is_container_v = is_container<T>::value;

	namespace iter_tools
	{
		//increment and decrement each of the argument values
		template<typename ...Args>
		constexpr void increment_args(Args& ...args) noexcept
		{
			(++args, ...);
			return;
		}

		template<typename ...Args>
		constexpr void decrement_args(Args& ...args) noexcept
		{
			(--args, ...);
			return;
		}

		// dereference each of the args and return as a tuple
		template<typename ...Args>
		constexpr auto expand_iters(Args ...args) noexcept
		{
			return std::forward_as_tuple(*args...);
		}
	}

	template<typename ...Iterators>
	class zip_iterator
	{
	public:
		using tuple_t = std::tuple<Iterators...>;

		using reference = std::tuple<typename std::iterator_traits<Iterators>::reference...>;
		using value_type = reference;
		using pointer = value_type*;
		using difference_type = std::common_type_t<typename std::iterator_traits<Iterators>::difference_type...>;
		// common iterator type, we wont allow better that bidirectional
		// since contigious strategies wont work with the adapter
		using iterator_category = std::common_type_t<std::bidirectional_iterator_tag,
			typename std::iterator_traits<Iterators>::iterator_category...>;

		//constexpr zip_iterator() noexcept = default;
		constexpr zip_iterator(Iterators... args) noexcept : _iters{ args... } {}

		constexpr zip_iterator& operator++() noexcept
		{
			std::apply(iter_tools::increment_args<Iterators...>, _iters);
			return *this;
		}

		constexpr zip_iterator& operator--() noexcept
		{
			std::apply(iter_tools::decrement_args<Iterators...>, _iters);
			return;
		}

		constexpr bool operator<(const zip_iterator& rhs) noexcept
		{
			return _iters < rhs._iters;
		}

		constexpr reference operator*() noexcept
		{
			return std::apply(iter_tools::expand_iters<Iterators...>, _iters);
		}

		constexpr tuple_t& get_iterator_tuple() noexcept
		{
			return _iters;
		}

		//TODO: swap

		constexpr bool operator==(const zip_iterator& other) const
		{
			return _iters == other._iters;
		}

		constexpr bool operator!=(const zip_iterator& other) const
		{
			return _iters != other._iters;
		}

	private:
		tuple_t _iters;
	};

	namespace container_tools
	{
		template<typename ...Containers>
		constexpr auto get_begin(Containers& ...conts) noexcept
		{
			return zip_iterator{ begin(conts)... };
		}

		template<typename ...Containers>
		constexpr auto get_end(Containers& ...conts) noexcept
		{
			return zip_iterator{ end(conts)... };
		}

		template<typename ...Containers>
		constexpr std::size_t get_size(Containers& ...conts) noexcept
		{
			return std::min({ size(conts), ... });
		}
	}

	template<typename ...Containers>
	class zip_container_adapter
	{
		using tuple_t = std::tuple<Containers&...>;
	public:
		constexpr zip_container_adapter(Containers& ...args) : _containers{ args... }
		{}

		auto begin()
		{
			return std::apply(container_tools::get_begin<Containers...>, _containers);
		}

		auto begin() const
		{
			return std::apply(container_tools::get_begin<Containers...>, _containers);
		}

		auto end()
		{
			return std::apply(container_tools::get_end<Containers...>, _containers);
		}

		auto end() const
		{
			return std::apply(container_tools::get_end<Containers...>, _containers);
		}

		std::size_t size() const
		{
			return std::apply(container_tools::get_size<Containers...>, _containers);
		}

	private:
		tuple_t _containers;
	};
}
