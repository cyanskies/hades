#ifndef HADES_RENDER_INSTANCE_HPP
#define HADES_RENDER_INSTANCE_HPP

#include <unordered_map>

#include "hades/export_curves.hpp"
#include "hades/level_interface.hpp"
#include "hades/level.hpp"
#include "hades/parallel_jobs.hpp"
#include "hades/timers.hpp"

namespace hades
{
	class render_instance
	{
	public:
		render_instance();
		//TODO: should this sync the time between server client?
		// YES, since our curve data is all still in server time
		void input_updates(const exported_curves &input);
		void make_frame_at(time_point t, render_implementation *mission, render_interface &output);
		render_implementation *get_interface();

	private:
		time_point _current_frame;
		time_point _prev_frame{};

		render_implementation _game;
		job_system _jobs; //TODO: get threadcount from console
		//std::unordered_map<unique_id, system_data_t> _system_data;
	};
}

#endif //HADES_GAMEINSTANCE_HPP