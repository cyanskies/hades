#include <cassert>
#include <mutex>
#include <type_traits>

namespace hades
{
	namespace detail
	{
		template<class T>
		struct Property : public Property_Base
		{
			explicit Property(const T &value);
			std::shared_ptr<std::atomic<T> > value;

			virtual std::string to_string();
		};

		template<class T>
		Property<T>::Property(const T &value) : value(std::make_shared<std::atomic<T> >(value)), Property_Base(typeid(T))
		{}

		template<class T>
		inline std::string Property<T>::to_string()
		{
			return std::to_string(value->load());
		}

		template<>
		struct Property<types::string> : public Property_Base
		{
			explicit Property(const std::string &value) : value(std::make_shared<value_guard<types::string>>(value)), Property_Base(typeid(types::string))
			{}

			console::property_str value;

			virtual types::string to_string()
			{
				return *value;
			}
		};
	}

	template<class T>
	struct valid_console_type : public std::false_type {};

	template<>
	struct valid_console_type<types::int32> : public std::true_type {};

	template<>
	struct valid_console_type<float> : public std::true_type {};

	template<>
	struct valid_console_type<bool> : public std::true_type {};

	template<>
	struct valid_console_type<types::string> : public std::true_type {};

	template<class T>
	bool Console::set(std::string_view identifier, const T &value)
	{
		//we can only store integral types in std::atomic
		//TODO: limit console property types to (int32, float, bool and string)
		static_assert(valid_console_type<T>::value, "Attempting to store an illegal type in the console.");

		std::shared_ptr<detail::Property_Base> out;
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		if(GetValue(identifier, out))
		{
			if (out->type != typeid(T))
				throw console::property_wrong_type("name: " + std::string(identifier) + ", value: " + to_string(value));
			else
			{
				std::shared_ptr<detail::Property<T> > stored = std::static_pointer_cast<detail::Property<T> >(out);
				*stored->value = value;
				return true;
			}
		}

		auto var = std::make_shared<detail::Property<T> >(value);

		TypeMap.insert(std::make_pair(identifier, var));

		return true;
	}

	template<class T>
	ConsoleVariable<T> Console::getValue(std::string_view var)
	{
		static_assert(valid_console_type<T>::value, "Attempting to get an illegal type from the console.");

		std::shared_ptr<detail::Property_Base > out;
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		if(GetValue(var, out))
		{
			if (out->type == typeid(T))
			{
				auto value = std::static_pointer_cast<detail::Property<T>> (out);
				return value->value;
			}
			else
				throw console::property_wrong_type("name: " + to_string(var) + ", requested type: " + 
					to_string(typeid(T).name()) + ", stored(actual) type: " + to_string(out->type.name()));
		}

		return nullptr;
	}
}