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
		return _gui.handleEvent(event);
	}

	void State::setGuiTarget(sf::RenderTarget &target)
	{
		_gui.setWindow(target);
	}

	void State::drawGui()
	{
		_gui.draw();
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
}//hades