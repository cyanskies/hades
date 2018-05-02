#ifndef HADES_UTIL_UNIQUEID_HPP
#define HADES_UTIL_UNIQUEID_HPP

#include <cassert>
#include <limits>

#include "hades/types.hpp"

namespace hades
{
	template<typename id_type>
	class unique_id_t
	{
	public:
		using type = id_type;

		unique_id_t() : _value(_count++) 
		{
			assert(_count != std::numeric_limits<type>::max());
		}

		unique_id_t(const unique_id_t&) = default;
		unique_id_t(unique_id_t&&) = default;
		unique_id_t &operator=(const unique_id_t&) = default;

		bool operator==(const unique_id_t &rhs) const
		{
			return _value == rhs._value;
		}

		bool operator!=(const unique_id_t &rhs) const
		{
			return !(_value == rhs._value);
		}

		type get() const { return _value; }

		template<typename T>
		friend bool operator<(const unique_id_t<T>&, const unique_id_t<T>&);

		static const unique_id_t<id_type> zero;
	private:
		//initialise the static counter with the types smallest value
		static type _count;
		type _value;
	};

	template<typename id_type>
	id_type unique_id_t<id_type>::_count = std::numeric_limits<id_type>::min();

	template<typename id_type>
	const unique_id_t<id_type> unique_id_t<id_type>::zero;

	template<typename T>
	bool operator<(const unique_id_t<T>& lhs, const unique_id_t<T>& rhs)
	{
		return lhs._value < rhs._value;
	}

	//process wide unique id
	using unique_id = unique_id_t<hades::types::uint64>;
}

namespace std {
	template <> struct hash<hades::unique_id>
	{
		size_t operator()(const hades::unique_id& key) const
		{
			std::hash<hades::unique_id::type> h;
			return h(key.get());
		}
	};
}

#endif // hades_util_unique_id_hpp
