#ifndef HADES_EXCEPTIONS_HPP
#define HADES_EXCEPTIONS_HPP

#include <exception>
#include <stdexcept>

// TODO: move to util, so that util can use these too
//defines commonly used exceptions

namespace hades
{
	//base class for errors that can occur at runtime
	//can be used to catch only hades runtime errors
	class runtime_error : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	class out_of_range_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	//thrown when an argument is out of range, null or otherwise refused by the function
	//NOTE: these are programmer errors, and shouldn't be handled by normal program execution
	class invalid_argument : public std::invalid_argument
	{
	public:
		using std::invalid_argument::invalid_argument;
	};

	//don't try to catch this, thrown for programmer errors, by functions that
	// are never meant to be called, like invalid SFINAE branches
	class logic_error : public std::logic_error
	{
	public:
		using std::logic_error::logic_error;
	};

	//thrown when a global function depends on a previously registered backend provider
	//when using the provided app class or hades_main this should never been thrown
	// as the built-in app setup should register all of these for you
	class provider_unavailable : public logic_error
	{
	public:
		using logic_error::logic_error;
	};
}

#endif //!HADES_EXCEPTIONS_HPP