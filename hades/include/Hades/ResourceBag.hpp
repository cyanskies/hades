#ifndef HADES_RESOURCEBAG_HPP
#define HADES_RESOURCEBAG_HPP

#include <functional>

#include "Hades/type_erasure.hpp"
#include "Hades/Types.hpp"
#include "Hades/UniqueId.hpp"

namespace hades
{
	namespace data
	{
		class data_manager;
		class resource_base;

		using loaderFunc = std::function<void(resource_base*, data_manager*)>;

		//type erased holder for a resource type
		class resource_base : public type_erasure::type_erased_base
		{
		public:
			void load(data_manager*);

			using type_erasure::type_erased_base::get_type;

			resource_base() = default;

		private:
			//incremented if the data is reloaded.
			types::uint8 _generation = 1;
			loaderFunc _resourceLoader;
		};

		//holds a resource type
		//resource types hold information nessicary to load a resource
		template<class T>
		class resource : public resource_base, public type_erasure::type_erased<T>
		{
		public:
			using type_base = resource_base;
			using type_erasure::type_erased<T>::get_type_static;
			using type_erasure::type_erased<T>::set;
			using type_erasure::type_erased<T>::get;

			type_erasure::type_erased_base::size_type get_type() const
			{
				return type_erasure::type_erased<T>::get_type();
			}

			resource() = default;
		};

		template<class T>
		class Resource
		{
		public:
			bool is_ready() const;
			T const * const get() const;

		private:
			resource<T> *_r;
			types::uint8 _generation = 0;
		};

		using resource_bag = property_bag<UniqueId, resource_base>;

		template<class T> 
		T get_resource(const resource_bag& bag, resource_bag::key_type key)
		{
			return bag.get<T, resource>(key);
		}

		template<class T> 
		void set_resource(resource_bag& bag, resource_bag::key_type key, const T& value)
		{
			bag.set<T, resource>(key, value);
		}
	}

}

#endif // hades_resourcebag_hpp
