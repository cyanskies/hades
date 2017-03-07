#ifndef HADES_PROPERTIES_HPP
#define HADES_PROPERTIES_HPP

#include <atomic>
#include <memory>

#include "Hades/Types.hpp"
#include "Hades/value_guard.hpp"

//Properties is a collection of global key->value pairs
namespace hades
{
	namespace console
	{
		template<class T>
		using property = std::shared_ptr<std::atomic<T>>;

		using property_str = std::shared_ptr<value_guard<types::string>>;

		class properties {
		public:
			virtual ~properties() {}

			//assigns the value to the id
			//returns false if the type was wrong for that identifier.
			virtual bool set(const types::string&, types::int32) = 0;
			virtual bool set(const types::string&, float) = 0;
			virtual bool set(const types::string&, bool) = 0;
			virtual bool set(const types::string&, const types::string&) = 0;

			//returns a ptr to the value of that id
			//returns null if the id is unused or is of the wrong type
			virtual property<types::int32> getInt(const types::string&) = 0;
			virtual property<float> getFloat(const types::string&) = 0;
			virtual property<bool> getBool(const types::string&) = 0;
			virtual property_str getString(const types::string&) = 0;

			//TODO: exists + erase
		};

		extern properties *property_provider;

		template<class T>
		bool setProperty(const types::string &name, const T &value)
		{
			if (property_provider)
				return property_provider->set(name, value);
			else 
				return false;
		}

		//returns the stored value or 'default' if the value doesn't exist(or no property provider registered)
		property<types::int32> getInt(const types::string&, types::int32);
		property<float> getFloat(const types::string&, float);
		property<bool> getBool(const types::string&, bool);
		property_str getString(const types::string&, const types::string&);
	}
}//hades

#endif //HADES_PROPERTIES_HPP