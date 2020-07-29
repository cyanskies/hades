namespace hades
{
	template<typename Push, typename PushUnder>
	inline void state::state_manager_callbacks(Push push, PushUnder push_under)
	{
		_push_state = push;
		_push_state_under = push_under;
	}
}
