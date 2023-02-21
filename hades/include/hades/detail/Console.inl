#include <cassert>
#include <mutex>
#include <type_traits>

namespace hades
{
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
	void Console::setValue(std::string_view identifier, const T &value)
	{
		//we can only store integral types in std::atomic
		static_assert(valid_console_type<T>::value, "Attempting to store an illegal type in the console.");

		detail::Property out;
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		if(GetValue(identifier, out))
		{
			using ValueType = std::decay_t<decltype(value)>;

			std::visit([identifier, &value](auto &&arg) {
				using U = std::decay_t<decltype(arg)>;
                using W = std::decay_t<typename U::element_type::value_type>;
				if constexpr(std::is_same_v<ValueType, W>)
					*arg = value;
				else
					throw console::property_wrong_type("name: " + to_string(identifier) + ", value: " + to_string(value));
			}, out);

			return;
		}
		throw console::property_missing(to_string(identifier));
	}

	template<class T>
	ConsoleVariable<T> Console::getValue(std::string_view var)
	{
		static_assert(valid_console_type<T>::value, "Attempting to get an illegal type from the console.");

		detail::Property out;
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		if(GetValue(var, out))
		{
			return std::visit([var](auto &&arg)->console::property<T> {
				using U = std::decay_t<decltype(arg)>;
				if constexpr(std::is_same_v<console::property<T>, U>)
					return arg;
				else
					throw console::property_wrong_type("name: " + to_string(var));
			}, out);
		}
		
		return nullptr;
	}
}
