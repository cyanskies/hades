#include "hades/system.hpp"

#include <algorithm>

#include "hades/utility.hpp"

namespace hades
{
	command make_command(std::string_view sv)
	{
		auto com = command{};
		if (auto pos = sv.find_first_of(' '); pos == std::string_view::npos)
		{
            return command{ sv, {} };
		}
		else
		{
			com.request = sv.substr(0, pos);
			sv = sv.substr(pos + 1, sv.size() - pos);
		}

		while (!sv.empty())
		{
			if (auto pos = sv.find_first_of(' '); pos == std::string_view::npos)
			{
				com.arguments.push_back(sv);
				break;
			}
			else
			{
				com.arguments.push_back(sv.substr(0, pos));
				sv = sv.substr(pos + 1, sv.size() - pos);
			}
		}

		return com;
	}

	bool operator==(const command& lhs, const command &rhs)
	{
		return lhs.request == lhs.request && lhs.arguments == rhs.arguments;
	}

	string to_string(const command& c)
	{
		using namespace std::string_literals;
		using namespace std::string_view_literals;
		return to_string(c.request) + " "s + to_string(std::begin(c.arguments), std::end(c.arguments), " "sv);
	}

	namespace console
	{
		previous_command::previous_command(command com) : request(com.request)
		{
			std::transform(std::begin(com.arguments), std::end(com.arguments), std::back_inserter(arguments), [](auto &&arg) {
				return hades::to_string(arg);
			});
		}

		bool operator==(const previous_command& lhs, const previous_command &rhs)
		{
			return lhs.request == rhs.request && lhs.arguments == rhs.arguments;
		}

		hades::string to_string(const previous_command &command)
		{
			using namespace std::string_view_literals;
			if (empty(command.arguments))
				return hades::to_string(command.request);
			else
			{
				return hades::to_string(command.request) + " " +
					hades::to_string(std::begin(command.arguments), std::end(command.arguments), " "sv);
			}
		}

		system* system_object = nullptr;

		bool add_function(std::string_view identifier, function func, bool replace, bool silent)
		{
			if(system_object)
				return system_object->add_function(identifier, func, replace, silent);

			return false;
		};

		bool add_function(std::string_view id, function_no_argument f, bool replace, bool silent)
		{
			if (system_object)
				return system_object->add_function(id, f, replace, silent);

			return false;
		}

		void erase_function(std::string_view identifier)
		{
			if (system_object)
				system_object->erase_function(identifier);
		}

		std::vector<std::string_view> get_function_names()
		{
			if (system_object)
				return system_object->get_function_names();

			return {};
		}

		bool run_command(const command &command)
		{
			if(system_object)
				return system_object->run_command(command);

			return false;
		}

		command_history_list command_history()
		{
			if (system_object)
				return system_object->command_history();

			return command_history_list{};
		}
	}
}//hades
