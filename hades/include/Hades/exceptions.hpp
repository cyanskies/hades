#ifndef HADES_EXCEPTIONS_HPP
#define HADES_EXCEPTIONS_HPP

#include <exception>
#include <stdexcept>

//defines commonly used exceptions

namespace hades
{
	//thrown when an argument is out of range, null or otherwise refused by the function
	class invalid_argument : public std::invalid_argument
	{
	public:
		using std::invalid_argument::invalid_argument;
		using std::invalid_argument::what;
	};

	//thrown when a global function depends on a previously registered backend provider
	//when using the provided app class or hades_main this should never been thrown
	// as the built-in app setup should register all of these for you
	class provider_unavailable : public std::logic_error
	{
	public:
		using std::logic_error::logic_error;
		using std::logic_error::what;
	};
}

#endif //!HADES_EXCEPTIONS_HPP