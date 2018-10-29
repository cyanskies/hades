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

		//TODO: does this still need to be shared?
		// would be good to just be property*
		template<class T>
		using property = std::shared_ptr<std::atomic<T>>;
	
		using property_int = property<types::int32>;
		using property_float = property<float>;
		using property_bool = property<bool>;
		using property_str = std::shared_ptr<value_guard<types::string>>;

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
			if(property_provider)
				return property_provider->create(s, v);
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