#ifndef HADES_STITCHES_HPP
#define HADES_STITCHES_HPP

#include <functional>
#include <memory>

#include "Aurora/Tools.hpp"

namespace hades
{
	class Console;
	class ResourceManager;
	class ScriptManager;
	class State;
	class StateManager;

	template<class T>
	class Uses
	{
	public:
		void attach(std::shared_ptr<T> t) {}
	private:
		Uses();
	};

	template<>
	class Uses<Console>
	{
	public:
		void attach(std::shared_ptr<Console> console)
		{
			this->console = console;
		}

	protected:
		std::shared_ptr<Console> console;
	};

	template<>
	class Uses<ResourceManager>
	{
	public:
		void attach(std::shared_ptr<ResourceManager> resource)
		{
			this->resource = resource;
		}

	protected:
		std::shared_ptr<ResourceManager> resource;
	};

	class Uses_StateManager
	{
	public:
		void attach(std::function<void(std::unique_ptr<State>&)> push)
		{
			pushState = push;
		}

	protected:
		std::function<void(std::unique_ptr<State>&)> pushState;
	};

	class Common_Uses : public Uses<Console>, public Uses<ResourceManager>
	{
	public:
		using Uses<Console>::attach;
		using Uses<ResourceManager>::attach;
	};

	class State_Uses : public Uses<Console>, public Uses<ResourceManager>, public Uses_StateManager
	{
	public:
		using Uses<Console>::attach;
		using Uses<ResourceManager>::attach;
		using Uses_StateManager::attach;
	};
}//hades

#endif //HADES_STITCHES_HPP