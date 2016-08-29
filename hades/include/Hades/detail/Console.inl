#include <cassert>
#include <mutex>
#include <type_traits>

namespace hades
{
	namespace detail
	{
		template<class T>
		Console_Type_Variable<T>::Console_Type_Variable(const T &value) : value(std::make_shared<std::atomic<T> >(value))
		{
			type = typeid(T);
		}

		template<class T>
		inline std::string Console_Type_Variable<T>::to_string()
		{
			return std::to_string(value->load());
		}
	}

	template<class T>
	bool Console::set(const std::string &identifier, const T &value)
	{
		
		//we don't want to set a value to a function ptr
		assert(typeid(T) != typeid(detail::Console_Type_Function));

		//we can only store integral types in std::atomic
		assert(std::is_integral<T>::value || typeid(T) == typeid(std::string));

		std::shared_ptr<detail::Console_Type_Base> out;
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		if(GetValue(identifier, out))
		{
			if(out->type != typeid(T))
				return false;
			else
			{
				std::shared_ptr<detail::Console_Type_Variable<T> > stored = std::static_pointer_cast<detail::Console_Type_Variable<T> >(out);
				stored->value->store(value);
				return true;
			}
		}

		auto var = std::make_shared<detail::Console_Type_Variable<T> >(value);

		TypeMap.insert(std::make_pair(identifier, var));

		return true;
	}

	template<class T>
	ConsoleVariable<T> Console::getValue(const std::string &var)
	{
		
		std::shared_ptr<detail::Console_Type_Base > out;
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		if(GetValue(var, out))
		{
			if(out->type == typeid(T))
			{
				auto value = std::static_pointer_cast<detail::Console_Type_Variable<T> > (out);
				return value->value;
			}
			else
				return nullptr;
		}

		return nullptr;
	}
}