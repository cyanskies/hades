#ifndef HADES_TYPEERASURE_HPP
#define HADES_TYPEERASURE_HPP

#include <memory>
#include <unordered_map>

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
			using type_base = type_erased_base;

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
		typename type_erased_base::size_type type_erased<T>::_typeId = type_count++;

		template<class T>
		class type_erased_simple : public type_erased<T>, type_erased_base 
		{};
	}

	template<class Key, class type_base = type_erasure::type_erased_base>
	class property_bag
	{
	public:
		template<class T, template<typename> class type_holder = type_erasure::template type_erased_simple>
		void set(Key key, T &value);

		template<class T, template<typename> class type_holder = type_erasure::template type_erased_simple>
		T const &get(Key key) const;

	private:
		std::unordered_map<Key, std::unique_ptr<type_base>> _bag;
	};
}

#include "Hades/detail/type_erasure.inl"

#endif // !HADES_TYPEERASURE_HPP
