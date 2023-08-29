#ifndef HADES_UTIL_TABLE_HPP
#define HADES_UTIL_TABLE_HPP

#include <functional>
#include <type_traits>
#include <vector>

#include "hades/utility.hpp"
#include "hades/vector_math.hpp"

//a type representing an arbitarily sized table
//supports adding the contents of tables together

//intended usage is for pathfinding system, ability to add multiple cost maps together to get the sum cost for a specific tile

namespace hades {
	using table_index_t = vector2_int;

	template<typename T>
	class basic_table 
	{
	public:
		using value_type = T;
		using index_type = table_index_t;
		using size_type = table_index_t::value_type;

		constexpr basic_table() noexcept = default;
		constexpr basic_table(index_type o, index_type s) noexcept(std::is_nothrow_constructible_v<index_type, index_type>)
            : _size{ s }, _offset{ o } {}
		virtual ~basic_table() noexcept = default;

		virtual value_type operator[](index_type) const = 0;
		virtual value_type operator[](size_type) const = 0;

        index_type position() const noexcept { return _offset; }
		void set_position(index_type p) noexcept
		{ _offset = p; }
		index_type size() const noexcept { return _size; }

	protected:
		std::size_t _index(index_type) const noexcept;
		std::size_t _index(size_type) const noexcept;

	private:
        index_type _size, _offset = {};
	};

	template<typename T>
	class always_table final : public basic_table<T>
	{
	public:
		using value_type = T;
        using base_type = basic_table<value_type>;
        using index_type = typename basic_table<value_type>::index_type;
        using size_type = typename basic_table<value_type>::size_type;

		template<typename U, std::enable_if_t<std::is_same_v<std::decay_t<U>, T>, int> = 0>
		constexpr always_table(index_type position, index_type size, U&& value)
			noexcept(std::is_nothrow_constructible_v<base_type, index_type, index_type>
				&& std::is_nothrow_constructible_v<T, decltype(std::forward<U>(value))>)
            : base_type{ position, size }, _always_value{ std::forward<U>(value) }
		{}

		constexpr value_type operator[](const index_type) const noexcept override
		{
			return _always_value;
		}

		constexpr value_type operator[](const size_type) const noexcept override
		{
			return _always_value;
		}

	private:
		T _always_value;
	};

	//main table class, can be added together to compound tables
	template<typename T>
	class table final : public basic_table<T>
	{
	public:
		using base_type = basic_table<T>;
		using value_type = typename basic_table<T>::value_type;
		using index_type = typename basic_table<T>::index_type;
		using size_type = typename index_type::value_type;

		table() noexcept = default;
		table(index_type position, index_type size, value_type value) : base_type{position, size},
            _data(integer_cast<std::size_t>(size.x*size.y), value) {}
		table(const table&) = default;
		table(const basic_table<T>&, value_type value);
		table(table&&) noexcept = default;

		table& operator=(const table&) = default;
		table& operator=(table&&) noexcept = default;

		void set(index_type i, value_type v);
		void set(size_type s, value_type v);

		typename std::vector<value_type>::reference operator[](index_type);
		value_type operator[](index_type) const override;

		typename std::vector<value_type>::reference operator[](size_type);
		value_type operator[](size_type) const override;

		std::vector<value_type> &data() noexcept
		{
			return _data;
		}

		const std::vector<value_type> &data() const noexcept
		{
			return _data;
		}

	private:
		std::vector<value_type> _data;
	};

	// wraps an array or vector in the table interface
	template<typename T>
	class table_view final : public basic_table<T>
	{
	public:
		using value_type = T;
		using base_type = basic_table<value_type>;
		using index_type = typename basic_table<value_type>::index_type;
		using size_type = typename basic_table<value_type>::size_type;

		template<typename U, std::enable_if_t<std::is_same_v<std::decay_t<U>, T>, int> = 0>
		constexpr table_view(index_type position, index_type size, const U* data)
			noexcept(std::is_nothrow_constructible_v<base_type, index_type, index_type>)
			: base_type{ position, size }, _data{ data }
		{}

		constexpr table_view(index_type position, index_type size, const std::vector<T>& data)
			noexcept(std::is_nothrow_constructible_v<base_type, index_type, index_type>)
			: base_type{ position, size }, _data{ data.data() }
		{
			assert(std::size(data) == (integer_cast<std::size_t>(size.x) * size.y));
		}

		value_type operator[](const index_type) const override;
		value_type operator[](const size_type) const override;

	private:
		const T* _data;
	};

	template<typename T, typename BinaryOp = std::plus<T>>
	class table_reduce_view final : public basic_table<T>
	{
	public:
		using base_type = basic_table<T>;
		using value_type = typename basic_table<T>::value_type;
		using index_type = typename basic_table<T>::index_type;
		using size_type = typename basic_table<T>::size_type;

		table_reduce_view(index_type pos, index_type size, value_type defaul,
			BinaryOp op = BinaryOp{});

		void add_table(const basic_table<T>&);

		value_type operator[](index_type) const override;
		value_type operator[](size_type) const override;
	private:
		std::vector<std::reference_wrapper<const basic_table<T>>> _tables;
		BinaryOp _op;
		value_type _default_val;
	};

	template<typename T, typename BinaryOp>
	table_reduce_view(auto pos, auto size, T defaul,
		BinaryOp op) -> table_reduce_view<T, BinaryOp>;
}

#include "detail/table.inl"

#endif // !HADES_UTIL_TABLE_HPP
