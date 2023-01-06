#include "hades/properties.hpp"

#include <cassert>
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>

#include "hades/exceptions.hpp"

using namespace std::string_literals;

namespace hades
{
	namespace console
	{
		class fallback_property_provider : public properties {
		public:
			using prop = std::variant<console::property_int, console::property_bool,
				console::property_float, console::property_str>;

			std::vector<std::string_view> get_property_names() const override
			{
				auto ret = std::vector<std::string_view>{};
				ret.reserve(size(_property_map));
				std::ranges::transform(_property_map, back_inserter(ret), &decltype(_property_map)::value_type::first);
				return ret;
			}

			void create(std::string_view name, int32 default_val, bool locked) override
			{
				_create(name, default_val, locked);
				return;
			}

			void create(std::string_view name, float default_val, bool locked) override
			{
				_create(name, default_val, locked);
				return;
			}

			void create(std::string_view name, bool default_val, bool locked) override
			{
				_create(name, default_val, locked);
				return;
			}

			void create(std::string_view name, std::string_view default_val, bool locked) override
			{
				_create(name, default_val, locked);
				return;
			}

			void lock_property(std::string_view name) override
			{
				const auto lock = std::scoped_lock{ _prop_mutex };
				auto iter = _property_map.find(name);
				if (iter == end(_property_map))
					return;

				std::visit([](auto&& prop) {
					prop->lock(true);
					return;
					}, iter->second);
				return;
			}

			void set(std::string_view name, types::int32 val) override
			{
				auto i = getInt(name);
				i->store(val);
				return;
			}

			void set(std::string_view name, float val) override
			{
				auto i = getFloat(name);
				i->store(val);
				return;
			}

			void set(std::string_view name, bool val) override
			{
				auto i = getBool(name);
				i->store(val);
				return;
			}

			void set(std::string_view name, std::string_view val) override
			{
				auto i = getString(name);
				i->store(to_string(val));
				return;
			}

			property_int getInt(std::string_view name) override
			{
				const auto lock = std::scoped_lock{ _prop_mutex };
				auto iter = _property_map.find(name);
				if (iter == end(_property_map))
					return nullptr;

				return get_property<int32>(iter->second);
			}

			property_float getFloat(std::string_view name) override
			{
				const auto lock = std::scoped_lock{ _prop_mutex };
				auto iter = _property_map.find(name);
				if (iter == end(_property_map))
					return nullptr;

				return get_property<float>(iter->second);
			}

			property_bool getBool(std::string_view name) override
			{
				const auto lock = std::scoped_lock{ _prop_mutex };
				auto iter = _property_map.find(name);
				if (iter == end(_property_map))
					return nullptr;

				return get_property<bool>(iter->second);
			}

			property_str getString(std::string_view name) override
			{
				const auto lock = std::scoped_lock{ _prop_mutex };
				auto iter = _property_map.find(name);
				if (iter == end(_property_map))
					return nullptr;

				return get_property<string>(iter->second);
			}

		private:
			template<typename T>
			void _create(std::string_view name, T val, bool locked)
			{
				const auto lock = std::scoped_lock{ _prop_mutex };
				auto iter = _property_map.find(name);

				using Ty = std::conditional_t<string_type<T>, string, T>;
				if (iter != std::end(_property_map))
				{
					try
					{
						if (get_value<Ty>(iter->second) == val)
							return;
					}
					catch (const console::property_wrong_type&)
					{ 
						/* error is covered by the exception thrown below */
					}

					throw console::property_name_already_used{ "Cannot create property; name: "s +
					to_string(name) + ", has already been used"s };
				}

				
				auto var = console::detail::make_property(Ty{ val }, locked);
				_property_map.emplace(to_string(name), std::move(var));
			}

			template<typename T>
			static T get_value(prop& p)
			{
				return get_property<T>(p)->load();
			}

			template<typename T>
			static console::property<T> get_property(prop& p)
			{
				try
				{
					return std::get<console::property<T>>(p);
				}
				catch (const std::bad_variant_access&)
				{
					throw console::property_wrong_type{ "Tried to access a console property with the wrong type."s };
				}
			}

			std::mutex _prop_mutex;
			unordered_map_string<prop> _property_map;
		};

		properties *property_provider = nullptr;

		void set_property_provider(properties* p)
		{
			assert(property_provider == nullptr);
			property_provider = p;
		}

		void use_fallback_property_provider()
		{
			assert(property_provider == nullptr);
			static auto provider = fallback_property_provider{};
			property_provider = &provider;
		}

		namespace detail
		{
			properties* get_property_provider() noexcept
			{
				return property_provider;
			}

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
