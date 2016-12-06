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
			key_null(const char* what) : std::runtime_error(what) {}
		};

		//the requested resource isn't of the type it is claimed to be
		class value_wrong_type : public std::runtime_error
		{
		public:
			value_wrong_type(const char* what) : std::runtime_error(what) {}
		};

		class type_erased_base
		{
		public:
			using size_type = types::uint8;

			virtual ~type_erased_base() {}

			virtual size_type get_type() const = 0;
		};

		template<class T>
		class type_erased : public type_erased_base
		{
		public:
			using type_base = type_erased_base;

			T &get() const { return &_data; }

			void set(T value) { _data = std::move(value); }

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
	}

	template<class Key>
	class property_bag
	{
	public:
		using key_type = Key;
		using base_type = type_erasure::type_erased_base;
		using data_map = std::unordered_map<key_type, std::unique_ptr<base_type>>;
		using iterator = typename data_map::iterator;
		using value_type = base_type;
		using ptr = base_type*;

		template<class T>
		void set(Key key, T value);

		template<class T>
		T get(Key key) const;

		template<class T>
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
