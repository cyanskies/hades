#include <cassert>
#include <mutex>
#include <type_traits>

namespace hades
{
	namespace detail
	{
		template<>
		struct Property<std::string> : public Property_Base
		{
			explicit Property(const std::string &value) : value(std::make_shared<String>(value)), Property_Base(typeid(std::string))
			{}

			std::shared_ptr<String> value;

			virtual std::string to_string()
			{
				return value->load();
			}
		};

		template<class T>
		Property<T>::Property(const T &value) : value(std::make_shared<std::atomic<T> >(value)), Property_Base(typeid(T))
		{}

		template<class T>
		inline std::string Property<T>::to_string()
		{
			return std::to_string(value->load());
		}
	}

	template<class T>
	bool Console::set(const std::string &identifier, const T &value)
	{
		//we can only store integral types in std::atomic
		static_assert(types::hades_type<T>(), "Attempting to store an illegal type in the console.");

		std::shared_ptr<detail::Property_Base> out;
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		if(GetValue(identifier, out))
		{
			if(out->type != typeid(T))
				return false;
			else
			{
				std::shared_ptr<detail::Property<T> > stored = std::static_pointer_cast<detail::Property<T> >(out);
				stored->value->store(value);
				return true;
			}
		}

		auto var = std::make_shared<detail::Property<T> >(value);

		TypeMap.insert(std::make_pair(identifier, var));

		return true;
	}

	template<class T>
	ConsoleVariable<T> Console::getValue(const std::string &var)
	{
		std::shared_ptr<detail::Property_Base > out;
		std::lock_guard<std::mutex> lock(_consoleVariableMutex);
		if(GetValue(var, out))
		{
			if(out->type == typeid(T))
			{
				auto value = std::static_pointer_cast<detail::Property<T> > (out);
				return value->value;
			}
			else
				return nullptr;
		}

		return nullptr;
	}
}