#include "hades/properties.hpp"

#include "hades/exceptions.hpp"

namespace hades
{
	namespace console
	{
		properties *property_provider = nullptr;

		namespace detail
		{
			template<class T, class U, class V>
			T get(std::string_view& name, U&& func, V default_value)
			{
				if (property_provider)
				{
					if (auto p = std::invoke(std::forward<U>(func), name))
						return p;
				}

				return detail::make_property<V>(default_value, false);
			}
		}

		constexpr auto provider_missing_error = "Property provider unavailable, use overloads with fall back values";

		std::vector<std::string_view> get_property_names()
		{
			if (property_provider)
				return property_provider->get_property_names();
			return {};
		}

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
			return detail::get<property<types::int32>>(name, [](std::string_view n) { return property_provider->getInt(n);}, default_value);
		}

		property_float get_float(std::string_view name, float default_value)
		{
			return detail::get<property<float>>(name, [](std::string_view n) { return property_provider->getFloat(n);}, default_value);
		}

		property_bool get_bool(std::string_view name, bool default_value)
		{
			return detail::get<property<bool>>(name, [](std::string_view n) { return property_provider->getBool(n);}, default_value);
		}

		property_str get_string(std::string_view name, std::string_view default_value)
		{
			return detail::get<property<string>>(name, [](std::string_view n) { return property_provider->getString(n); }, to_string(default_value));
		}
	}
}//hades
