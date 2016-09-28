#include "Hades/State.hpp"

namespace hades
{
	State::State() : _alive(true), _init(false), _paused(false), _active(true)
	{}

	State::~State()
	{}

	bool State::isAlive() const
	{
		return _alive;
	}

	bool State::isInit() const
	{
		return _init;
	}

	void State::initDone()
	{
		_init = true;
	}

	void State::kill()
	{
		_alive = false;
	}

	void State::activate()
	{
		_active = true;
	}

	bool State::isActive() const
	{
		return _active;
	}

	void State::deactivate()
	{
		_active = false;
		dropFocus();
	}

	void State::grabFocus()
	{
		resume();
		_paused = false;	
	}

	CallbackSystem &State::getCallbackHandler()
	{
		return _callbacks;
	}

	void State::handleInput(thor::ActionContext<int> context, int id)
	{
		auto func = _stateBinds.find(id);

		if(func != _stateBinds.end())
			func->second(context);
	}

	bool State::paused() const
	{
		return _paused;
	}
	
	void State::dropFocus()
	{
		pause();
		_paused = true;
	}

	void State::bindCallback(int id, std::function<void(EventContext)> func)
	{
		_callbacks.connect(id, func);
	}

	void State::clearCallback(int id)
	{
		_callbacks.clearConnections(id);
	}

	void State::clearCallbacks()
	{
		_callbacks.clearAllConnections();
	}
}//hades