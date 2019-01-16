#ifndef HADES_PROPERTIES_HPP
#define HADES_PROPERTIES_HPP

#include <atomic>
#include <exception>
#include <memory>
#include <string_view>

#include "hades/exceptions.hpp"
#include "hades/types.hpp"
#include "hades/value_guard.hpp"

//Properties is a collection of global key->value pairs
namespace hades
{
	namespace console
	{
		//logic error, requesting the wrong type represents a code bug
		class property_wrong_type : public std::logic_error
		{
		public:
			using std::logic_error::logic_error;
		};

		class property_name_already_used : public std::logic_error
		{
		public:
			using std::logic_error::logic_error;
		};

		class property_missing : public std::logic_error
		{
		public:
			using std::logic_error::logic_error;
		};

		template<typename T>
		struct basic_property
		{
			basic_property(T val) : _value{val}, _default{val}
			{}

			basic_property &operator=(T val)
			{
				store(val);
				return *this;
			}

			operator T()
			{
				return load();
			}

			using value_type = T;

			T load()
			{
				return _value;
			}

			T load_default()
			{
				return _default;
			}

			void store(T desired)
			{
				_value = desired;
			}

			T compare_exchange(T &expected, T desired)
			{
				return _value.compare_exchange(expected, desired);
			}

		private:
			std::conditional_t<std::is_trivially_copyable_v<T>,
				std::atomic<T>, value_guard<T>> _value;
			T _default;
		};

		template<typename T>
		using property = std::shared_ptr<basic_property<T>>;
	
		template<typename T>
		property<T> make_property(T value)
		{
			return std::make_shared< basic_property<T> >(value);
		}

		using property_int = property<int32>;
		using property_float = property<float>;
		using property_bool = property<bool>;
		using property_str = property<types::string>;

		class properties {
		public:
			virtual ~properties() {}

			//creates the property and sets it's default value
			virtual void create(std::string_view, int32 default_val) = 0;
			virtual void create(std::string_view, float default_val) = 0;
			virtual void create(std::string_view, bool default_val) = 0;
			virtual void create(std::string_view, std::string_view default_val) = 0;

			//assigns the value to the id
			//throws property_wrong_type if the type doesn't match the property name
			virtual void set(std::string_view, types::int32) = 0;
			virtual void set(std::string_view, float) = 0;
			virtual void set(std::string_view, bool) = 0;
			virtual void set(std::string_view, std::string_view) = 0;

			//returns a ptr to the value of that id
			//returns nullptr if the property is undefined
			//throws property_wrong_type if the type doesn't match the property name
			virtual property_int getInt(std::string_view) = 0;
			virtual property_float getFloat(std::string_view) = 0;
			virtual property_bool getBool(std::string_view) = 0;
			virtual property_str getString(std::string_view) = 0;

			//TODO: exists + erase
		};

		//TODO: replace with set property ptr
		extern properties *property_provider;

		// The following functions will throw if the provider isn't available
		template<typename T>
		void create_property(std::string_view s, T v)
		{
			if (property_provider)
				return property_provider->create(s, v);
			else
				throw provider_unavailable{ "property provideder not available" };
		}

		// Property global functions are required to work even when the property provider is absent
		// as such SetProperty doesn't provide any success feedback
		// and Get* functions return the provided default if possible

		template<class T>
		void set_property(std::string_view name, const T &value)
		{
			if (property_provider)
				property_provider->set(name, value);
		}

		//returns the stored value or 'default' if the value doesn't exist(or no property provider registered)
		//if the requested type doesn't match the type stored then throws console::property_wrong_type
		property_int get_int(std::string_view);
		property_float get_float(std::string_view);
		property_bool get_bool(std::string_view);
		property_str get_string(std::string_view);

		property_int get_int(std::string_view, types::int32);
		property_float get_float(std::string_view, float);
		property_bool get_bool(std::string_view, bool);
		property_str get_string(std::string_view, std::string_view);
	}
}//hades

#endif //HADES_PROPERTIES_HPP