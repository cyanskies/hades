#include "Hades/State.hpp"

namespace hades
{
	State::State() : _alive(true), _init(false), _paused(false)
	{}

	State::~State()
	{}

	void State::setStateManangerCallbacks(State::push_func push, State::push_func push_under)
	{
		PushState = push; PushStateUnder = push_under;
	}

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

	void State::grabFocus()
	{
		resume();
		_paused = false;	
	}

	bool State::guiInput(sf::Event& event)
	{
		return false;
		//return _gui.HandleEvent(event);
	}

	void State::updateGui(sf::Time frameTime)
	{
		//_gui.Update(frameTime.asSeconds());
	}

	bool State::paused() const
	{
		return _paused;
	}
	
	void State::dropFocus()
	{
		pause();
		//states should rebuild their gui in reinit
		//_gui.RemoveAll();
		_paused = true;
	}
}//hades