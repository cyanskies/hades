#ifndef HADES_PROPERTIES_HPP
#define HADES_PROPERTIES_HPP

#include <atomic>
#include <memory>

#include "Hades/Types.hpp"

//Properties is a collection of global key->value pairs
namespace hades
{
	namespace console
	{
		template<class T>
		using property = std::shared_ptr<std::atomic<T>>;

		class properties {
			//assigns the value to the id
			//reurns false if the type was wrong for that identifier.
			virtual bool set(const types::string&, const types::int32&) = 0;
			virtual bool set(const types::string&, const float&) = 0;
			virtual bool set(const types::string&, const bool&) = 0;
			virtual bool set(const types::string&, const types::string&) = 0;

			virtual property<types::int32> getInt(const types::string&) = 0;
			virtual property<float> getFloat(const float&) = 0;
			virtual property<bool> getBool(const bool&) = 0;
			virtual property<types::string> getString(const types::string&) = 0;
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
		template<class T>
		property<T> getProperty(const types::string &name, const T &default)
		{
			if (property_provider)
			{
				if (auto p = property_provider->get(name))
					return p;
				else
				{
					if(setProperty(name, default))
						return property_provider->get(name);
				}
			}
			
			return std::make_shared<std::atomic<T>>(default);
		}
	}
}//hades

#endif //HADES_PROPERTIES_HPP