#ifndef HADES_UNIQUEID_HPP
#define HADES_UNIQUEID_HPP

#include <cassert>
#include <limits>

#include "Types.hpp"

namespace hades
{
	namespace data
	{
		template<typename id_type>
		class UniqueId_t
		{
		public:
			using type = id_type;

			UniqueId_t() : _value(_count++) 
			{
				assert(_count != std::numeric_limits<type>::max();
			}

			bool operator==(const UniqueId_t& rhs) const
			{
				return _value == rhs._value;
			}

			type get() const { return _value; }
		private:
			//initialise the static counter with the types smallest value
			static type _count = std::numeric_limits<type>::min();
			type _value;
		};

		template<typename id_type>
		id_type UniqueId_t<id_type>::_count = 0;

		//process wide unique id
		using UniqueId = UniqueId_t<hades::types::uint64>;
	}
}

namespace std {
	template <> struct hash<hades::data::UniqueId>
	{
		size_t operator()(const hades::data::UniqueId& key) const
		{
			std::hash<hades::data::UniqueId::type> h;
			return h(key.get());
		}
	};
}

#endif // hades_uniqueid_hpp
