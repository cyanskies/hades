#include "Hades/Properties.hpp"

namespace hades
{
	namespace console
	{
		properties *property_provider = nullptr;

		template<class T, class U, class V>
		T get(const types::string &name, U func, V default)
		{
			if (property_provider)
			{
				if (auto p = func(name))
					return p;
				else
				{
					if (SetProperty(name, default))
						return func(name);
				}
			}

			return std::make_shared<std::atomic<V>>(default);
		}

		property<types::int32> GetInt(const types::string &name, types::int32 default)
		{
			return get<property<types::int32>>(name, [](types::string n) { return property_provider->getInt(n);}, default);
		}

		property<float> GetFloat(const types::string &name, float default)
		{
			return get<property<float>>(name, [](types::string n) { return property_provider->getFloat(n);}, default);
		}

		property<bool> GetBool(const types::string &name, bool default)
		{
			return get<property<bool>>(name, [](types::string n) { return property_provider->getBool(n);}, default);
		}

		property_str GetString(const types::string &name, const types::string &default)
		{
			if (property_provider)
			{
				if (auto p = property_provider->getString(name))
					return p;
				else
				{
					if (SetProperty(name, default))
						return property_provider->getString(name);
				}
			}

			return std::make_shared<value_guard<types::string>>(default);
		}
	}
}//hades