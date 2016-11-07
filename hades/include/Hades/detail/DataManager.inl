#include "Hades/Types.hpp"
#include "Hades/type_erasure.hpp"

namespace hades
{
	template<typename id_type>
	class UniqueId_t
	{
	public:
		UniqueId_t(id_type value) : _value(value) {}

		bool operator==(const UniqueId_t& rhs) const
		{
			return _value == rhs._value;
		}

	private:
		id_type _value;
	};

	template<typename T>
	bool operator<(const UniqueId_t<T>& lhs, const UniqueId_t<T>& rhs) 
	{
		return lhs._value < rhs._value;
	}

	namespace data
	{
		class typeholder_base : public type_erasure::type_erased_base
		{
		public:
			using CountType = types::uint16;

			virtual ~typeholder_base() {}

			CountType getGenCount() const;
		private:
			CountType _typeid, _genCount = 0;
			static CountType _nextTypeId;
		};

		template<typename T>
		class typeholder : public typeholder_base
		{
		public:
			void load();

			const T const* get() const;
		};

		template<typename T>
		class type
		{
		public:
			const T const* get() const;
		private:

		};
	}
}