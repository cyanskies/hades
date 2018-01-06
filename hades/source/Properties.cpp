#include "Hades/exceptions.hpp"
#include "Hades/Properties.hpp"

namespace hades
{
	namespace console
	{
		properties *property_provider = nullptr;

		template<class T, class U, class V>
		T get(std::string_view &name, U func, V default)
		{
			if (property_provider)
			{
				if (auto p = func(name))
					return p;
				else
				{
					SetProperty(name, default);
					return func(name);
				}
			}

			return std::make_shared<std::atomic<V>>(default);
		}

		property_int GetInt(std::string_view name, types::int32 default)
		{
			return get<property<types::int32>>(name, [](std::string_view n) { return property_provider->getInt(n);}, default);
		}

		property_float GetFloat(std::string_view name, float default)
		{
			return get<property<float>>(name, [](std::string_view n) { return property_provider->getFloat(n);}, default);
		}

		property_bool GetBool(std::string_view name, bool default)
		{
			return get<property<bool>>(name, [](std::string_view n) { return property_provider->getBool(n);}, default);
		}

		property_str GetString(std::string_view name, std::string_view default)
		{
			if (property_provider)
			{
				if (auto p = property_provider->getString(name))
					return p;
				else
				{
					SetProperty(name, default);
					return property_provider->getString(name);
				}
			}

			return std::make_shared<value_guard<types::string>>(to_string(default));
		}
	}
}//hades