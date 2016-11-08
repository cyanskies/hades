#ifndef HADES_TYPEERASURE_HPP
#define HADES_TYPEERASURE_HPP

#include "Hades/Types.hpp"

namespace hades {
	namespace type_erasure {
		class type_erased_base
		{
		public:
			using size_type = types::uint8;

			virtual ~type_erased_base() {}

			virtual size_type get_type() const = 0;
		};

		template<class T>
		class type_erased
		{
		public:
			virtual T const& get() const { return _data; }

			virtual void set(T &value) { _data = value; }

			virtual type_erased_base::size_type get_type() const {
				return _typeId;
			}

			static type_erased_base::size_type get_type_static() {
				return _typeId;
			}

		private:
			T _data;
			static type_erased_base::size_type _typeId;
		};

		namespace {
			typename type_erased_base::size_type type_count = 0;
		}
		
		template<class T>
		typename type_erased_base::size_type type_erased<T>::_typeId = ::type_count++;

		template<class T>
		class type_erased_simple : public type_erased<T>, type_erased_base 
		{};
	}
}

#endif // !HADES_TYPEERASURE_HPP
