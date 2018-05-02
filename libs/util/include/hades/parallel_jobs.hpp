#ifndef HADES_UTIL_PARALLELJOBS_HPP
#define HADES_UTIL_PARALLELJOBS_HPP

#include <functional>
#include <memory>

namespace hades {
	namespace parallel_jobs {
		struct job;

		//data base class, inherit job data from this to ensure it can be deleted safely
		struct job_data
		{
			virtual ~job_data() {}
		};

		using job_data_ptr = std::unique_ptr<job_data>;
		using job_function = std::function<bool(const job_data&)>;

		//returns true if the system is safe to start,
		//false if jobs are already running or the system has been joined/shut down
		bool ready();
		//create job
		//returns a non-owning ptr
		job* create(job_function function);
		job* create(job_function function, job_data_ptr user_data);
		//create child job
		job* create_child(job* parent, job_function function);
		job* create_child(job* parent, job_function function, job_data_ptr);
		//run job(add to job queue)
		void run(job* job);
		//wait for job to finish(do other jobs in mean time),
		//waiting causes the worker threads to start; the system is idle until then.
		void wait(const job* job);
		//end the frame, clean up all the jobs
		//a job ptr will remain valid until this is called
		void clear();

		//==system functions==
		//init the system, should only be called once per proccess
		void init();
		//end all the job systems threads; shuts down the system
		void join();
	}
}

#endif // !HADES_UTIL_PARALLELJOBS_HPP
