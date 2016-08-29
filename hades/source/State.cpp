#include "Hades/State.hpp"

namespace hades
{
	State::State() : _alive(true), _init(false), _paused(false), _active(true)
	{}

	State::~State()
	{}

	bool State::isAlive()
	{
		return _alive;
	}

	bool State::isInit()
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

	bool State::isActive()
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
		timers.resume();
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

	bool State::paused()
	{
		return _paused;
	}
	
	void State::dropFocus()
	{
		timers.pause();
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