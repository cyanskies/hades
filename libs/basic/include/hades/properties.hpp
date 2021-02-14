#ifndef HADES_PROPERTIES_HPP
#define HADES_PROPERTIES_HPP

#include <atomic>
#include <exception>
#include <memory>
#include <string_view>
#include <utility>

#include "hades/exceptions.hpp"
#include "hades/types.hpp"
#include "hades/value_guard.hpp"

//Properties is a collection of global key->value pairs
namespace hades
{
	namespace console
	{
		class property_error : public runtime_error
		{
		public:
			using runtime_error::runtime_error;
		};

		//logic error, requesting the wrong type represents a code bug
		class property_wrong_type : public property_error
		{
		public:
			using property_error::property_error;
		};

		class property_name_already_used : public property_error
		{
		public:
			using property_error::property_error;
		};

		class property_missing : public property_error
		{
		public:
			using property_error::property_error;
		};

		class property_locked : public property_error
		{
		public:
			using property_error::property_error;
		};

		template<typename T>
		struct basic_property
		{
			explicit basic_property(T val, bool locked = false) : _value{val}, _default{val}, _locked{locked}
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

			void lock(bool l) noexcept
			{
				_locked = l;
			}

			bool locked() noexcept
			{
				return _locked;
			}

			T load()
			{
				return _value.load(std::memory_order_relaxed);
			}

			T load_default() noexcept(std::is_nothrow_copy_constructible_v<T>)
			{
				return _default;
			}

			void store(T desired)
			{
				_value.store(desired, std::memory_order_release);
			}

		private:
			std::conditional_t<std::is_trivially_copyable_v<T>,
				std::atomic<T>, value_guard<T>> _value;
			T _default;
			bool _locked = false;
		};

		template<typename T>
		using property = std::shared_ptr<basic_property<T>>;
	
		namespace detail
		{
			template<typename T>
			property<T> make_property(T value, bool locked)
			{
				return std::make_shared< basic_property<T> >(value, locked);
			}
		}

		//TODO: use float32
		using property_int = property<int32>;
		using property_float = property<float>;
		using property_bool = property<bool>;
		using property_str = property<types::string>;

		class properties {
		public:
			virtual ~properties() noexcept = default;

			//creates the property and sets it's default value
			virtual void create(std::string_view, int32 default_val, bool locked = false) = 0;
			virtual void create(std::string_view, float default_val, bool locked = false) = 0;
			virtual void create(std::string_view, bool default_val, bool locked = false) = 0;
			virtual void create(std::string_view, std::string_view default_val, bool locked = false) = 0;

			virtual void lock_property(std::string_view) = 0;

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
		void create_property(std::string_view s, T v, bool locked = false)
		{
			if (property_provider)
                property_provider->create(s, std::forward<T>(v), locked);
			else
				throw provider_unavailable{ "property provider not available" };
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

		void lock_property(std::string_view);

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
