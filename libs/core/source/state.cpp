#include "hades/state.hpp"

namespace hades
{
	bool state::is_alive() const
	{
		return _alive;
	}

	bool state::is_init() const
	{
		return _init;
	}

	void state::init_done()
	{
		_init = true;
	}

	void state::kill()
	{
		_alive = false;
	}

	void state::grab_focus()
	{
		resume();
		_paused = false;	
	}

	bool state::paused() const
	{
		return _paused;
	}
	
	void state::drop_focus()
	{
		pause();
		//states should rebuild their gui in reinit
		//_gui.RemoveAll();
		_paused = true;
	}
}//hades