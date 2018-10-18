#include "hades/properties.hpp"

#include "hades/exceptions.hpp"
#include "hades/utility.hpp"

namespace hades
{
	namespace console
	{
		properties *property_provider = nullptr;

		template<class T, class U, class V>
		T get(std::string_view &name, U func, V default_value)
		{
			if (property_provider)
			{
				if (auto p = func(name))
					return p;
			}

			return std::make_shared<std::atomic<V>>(default_value);
		}

		constexpr auto provider_missing_error = "Property provider unavailable, use overloads with fall back values";

		property_int get_int(std::string_view name)
		{
			if (!property_provider)
				throw provider_unavailable{ provider_missing_error };

			return property_provider->getInt(name);
		}

		property_float get_float(std::string_view name)
		{
			if (!property_provider)
				throw provider_unavailable{ provider_missing_error };

			return property_provider->getFloat(name);
		}

		property_bool get_bool(std::string_view name)
		{
			if (!property_provider)
				throw provider_unavailable{ provider_missing_error };

			return property_provider->getBool(name);
		}

		property_str get_string(std::string_view name)
		{
			if (!property_provider)
				throw provider_unavailable{ provider_missing_error };
			
			return property_provider->getString(name);
		}

		property_int get_int(std::string_view name, types::int32 default_value)
		{
			return get<property<types::int32>>(name, [](std::string_view n) { return property_provider->getInt(n);}, default_value);
		}

		property_float get_float(std::string_view name, float default_value)
		{
			return get<property<float>>(name, [](std::string_view n) { return property_provider->getFloat(n);}, default_value);
		}

		property_bool get_bool(std::string_view name, bool default_value)
		{
			return get<property<bool>>(name, [](std::string_view n) { return property_provider->getBool(n);}, default_value);
		}

		property_str get_string(std::string_view name, std::string_view default_value)
		{
			if (property_provider)
			{
				if (auto p = property_provider->getString(name))
					return p;
				else
				{
					set_property(name, default_value);
					return property_provider->getString(name);
				}
			}

			return std::make_shared<value_guard<types::string>>(to_string(default_value));
		}
	}
}//hades
