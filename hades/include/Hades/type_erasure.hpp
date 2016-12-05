#ifndef HADES_TYPEERASURE_HPP
#define HADES_TYPEERASURE_HPP

#include <exception>
#include <memory>
#include <unordered_map>

#include "Hades/Types.hpp"

namespace hades {
	namespace type_erasure {
		class key_null : public std::runtime_error
		{
		public:
			using std::runtime_error(const char*);
		};

		//the requested resource isn't of the type it is claimed to be
		class value_wrong_type : public std::runtime_error
		{
		public:
			using std::runtime_error(const char*);
		};

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

			virtual T get() const { return _data; }

			virtual void set(const T &value) { _data = value; }

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
		class type_erased_simple : public type_erased<T>, public type_erased_base 
		{
		public:
			type_erasure::type_erased_base::size_type get_type() const
			{
				return type_erasure::type_erased<T>::get_type();
			}
		};
	}

	template<class Key, class type_base = type_erasure::type_erased_base>
	class property_bag
	{
	public:
		using key_type = Key;
		using base_type = type_base;
		using data_map = std::unordered_map<key_type, std::unique_ptr<type_base>>;
		using iterator = typename data_map::iterator;
		using value_type = type_base;
		using ptr = type_base*;

		template<class T, template<typename> class type_holder = type_erasure::template type_erased_simple>
		void set(Key key, const T& value);

		template<class T, template<typename> class type_holder = type_erasure::template type_erased_simple>
		T get(Key key) const;

		template<class T, template<typename> class type_holder = type_erasure::template type_erased_simple>
		ptr get_reference(Key key);

		iterator begin();
		iterator end();

		bool empty();

	private:
		
		data_map _bag;
	};
}

#include "Hades/detail/type_erasure.inl"

#endif // !HADES_TYPEERASURE_HPP
