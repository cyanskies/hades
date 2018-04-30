#ifndef HADES_EVENTS_HPP
#define HADES_EVENTS_HPP

#include "GameInterface.hpp"

#include "Hades/type_erasure.hpp"

namespace hades
{
	using event_t = var_bag<EntityId>;

	using event_list = std::vector<int>;

	class EventLog
	{
	public:
		void publish_event(EntityId target, void event, sf::Time t);

		void get_events(EntityId target);
	};
}

#endif //HADES_EVENTS_HPP
