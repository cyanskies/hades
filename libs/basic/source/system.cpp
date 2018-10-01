#include "hades/system.hpp"

#include <algorithm>

#include "hades/utility.hpp"

namespace hades
{
	command::command(std::string_view sv)
	{
		if (auto pos = sv.find_first_of(' '); pos == std::string_view::npos)
		{
			request = sv;
			return;
		}
		else
		{
			request = sv.substr(0, pos);
			sv = sv.substr(pos + 1, sv.size() - pos);
		}

		while (!sv.empty())
		{
			if (auto pos = sv.find_first_of(' '); pos == std::string_view::npos)
			{
				arguments.push_back(sv);
				return;
			}
			else
			{
				arguments.push_back(sv.substr(0, pos));
				sv = sv.substr(pos + 1, sv.size() - pos);
			}
		}
	}

	bool operator==(const command& lhs, const command &rhs)
	{
		return lhs.request == lhs.request && lhs.arguments == rhs.arguments;
	}

	types::string to_string(const command& c)
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

		types::string to_string(const previous_command &command)
		{
			return hades::to_string(command.request) + " " + 
				hades::to_string(std::begin(command.arguments), std::end(command.arguments));
		}

		system* system_object = nullptr;

		bool add_function(std::string_view identifier, function func, bool replace)
		{
			if(system_object)
				return system_object->add_function(identifier, func, replace);

			return false;
		};

		void erase_function(std::string_view identifier)
		{
			if (system_object)
				system_object->erase_function(identifier);
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
