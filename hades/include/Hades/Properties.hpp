#ifndef HADES_PROPERTIES_HPP
#define HADES_PROPERTIES_HPP

#include <atomic>
#include <exception>
#include <memory>
#include <string_view>

#include "Hades/exceptions.hpp"
#include "Hades/Types.hpp"
#include "Hades/value_guard.hpp"

//Properties is a collection of global key->value pairs
namespace hades
{
	namespace console
	{
		//logic error, requesting the wrong type represents a code bug
		class property_wrong_type : std::logic_error
		{
		public:
			using std::logic_error::logic_error;
			using std::logic_error::what;
		};

		template<class T>
		using property = std::shared_ptr<std::atomic<T>>;

		using property_int = property<types::int32>;
		using property_float = property<float>;
		using property_bool = property<bool>;
		using property_str = std::shared_ptr<value_guard<types::string>>;

		class properties {
		public:
			virtual ~properties() {}

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

		extern properties *property_provider;

		// Property global functions are required to work even when the property provider is absent
		// as such SetProperty doesn't provide any success feedback
		// and Get* functions return the provided default if possible

		template<class T>
		void SetProperty(std::string_view name, const T &value)
		{
			if (property_provider)
				property_provider->set(name, value);
		}

		//returns the stored value or 'default' if the value doesn't exist(or no property provider registered)
		//if the requested type doesn't match the type stored then throws console::property_wrong_type
		property_int GetInt(std::string_view, types::int32);
		property_float GetFloat(std::string_view, float);
		property_bool GetBool(std::string_view, bool);
		property_str GetString(std::string_view, std::string_view);
	}
}//hades

#endif //HADES_PROPERTIES_HPP