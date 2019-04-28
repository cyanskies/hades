#ifndef HADES_RENDER_INSTANCE_HPP
#define HADES_RENDER_INSTANCE_HPP

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
		void input_updates(const exported_curves &input);
		void make_frame_at(time_point t, render_interface&);

	private:
		render_implementation _game;

		job_system _jobs;
	};
}

#endif //HADES_GAMEINSTANCE_HPP