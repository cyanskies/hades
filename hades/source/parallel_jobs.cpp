#include "Hades/parallel_jobs.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "Hades/Types.hpp"
#include "Hades/Utility.hpp"

namespace hades {
	namespace parallel_jobs {

		//implementation based on article series
		//https://blog.molecular-matters.com/2015/08/24/job-system-2-0-lock-free-work-stealing-part-1-basics/

		struct job
		{
			job(job_function f) : function(f), parent_job(nullptr), unfinished_children(1), user_data(nullptr) {}

			job_function function;
			job* parent_job;
			std::atomic<types::int32> unfinished_children;
			job_data_ptr user_data;
		};

		//mutex to protect the joblist
		using mutex_type = std::mutex;
		mutex_type joblist_mutex;
		//storage for jobs
		std::vector<std::unique_ptr<job>> g_joblist;
		//contains the number of extra threads we have available(doesn't count the main thread)
		//note: hardware_concurrency can return 0 as an error condition.
		//std::max ensures the lowest value we can get is 0.
		const types::uint32 g_thread_count = static_cast<types::uint32>(std::max(static_cast<types::int32>(std::thread::hardware_concurrency()) - 1, 0));
		//thread id type
		using thread_id = std::size_t;
		using thread_idle_t = std::size_t;
		//the next unique thread id
		std::atomic<thread_id> g_thread_id_next = 0;
		//the system will set this to false to make all the threads joinable
		std::atomic_bool g_workers_enabled = true;
		//threads increment this when they have no work. if g_idle_threads == g_thread_count then the whole system is idle
		std::atomic<thread_idle_t> g_idle_threads = 0;
		//a pool of threads that we can use.
		using thread_pool = std::vector<std::thread>;
		thread_pool g_threads(g_thread_count);
		//a vector of mutexs to guard access to the per thread queues
		std::vector<std::mutex> g_worker_queue_mutexes(g_thread_count + 1);
		//each worker thread gets a queue to work with
		//the queue is in g_worker_job_queues[thread_id]
		using worker_queue = std::deque<job*>;
		std::vector<worker_queue> g_worker_job_queues(g_thread_count + 1);
		using lock_t = std::unique_lock<mutex_type>;
		using locked_queue = std::tuple<worker_queue*, lock_t>;
		bool joined = false;

		//==per thread data==
		//the job the thread is currently working on
		thread_local job* t_current_job;
		//the threads unique id(so it can look up it's queue in the job queue
		//workers are threads 0 - thread_count, main thread is thread_count + 1
		thread_local thread_id t_thread_id;
		//lets the thread ditermine if it is asleap
		thread_local bool t_sleep = false;

		void worker_init();
		void worker_function();

		//==system functions==
		void init()
		{
			//assert that the system isn't already running.
			assert(std::none_of(g_threads.begin(), g_threads.end(), [](const std::thread &t) {return t.joinable();}));
			//once the system has shut down that's it, no more init or rejoin
			assert(!joined);

			for (auto &t : g_threads)
				t = std::thread(worker_function);

			//initialise the main threads worker data too.
			worker_init();
		}

		void join()
		{
			//assert that the system is running.
			assert(std::all_of(g_threads.begin(), g_threads.end(), [](const std::thread &t) {return t.joinable();}));
			//once the system has shut down that's it, no more init or rejoin
			assert(!joined);

			g_workers_enabled = false;

			for (auto &t : g_threads)
			{
				assert(t.joinable());
				t.join();//TODO: catch invalid_argument exception from non-joinable thread(should only be possible if init was skipped);
			}

			joined = true;
		}

		//==client functions==
		job* create(job_function function)
		{
			assert(function);
			//create a new job and store it in the global jobstore
			auto newjob = std::make_unique<job>( function );
			job* jobptr = &*newjob;

			std::lock_guard<std::mutex> guard(joblist_mutex);
			g_joblist.push_back(std::move(newjob));

			//return ptr to the new job
			//non-owning reference pointer.
			return jobptr;
		}

		job* create(job_function function, job_data_ptr user_data)
		{
			assert(user_data);
			auto j = create(function);
			j->user_data.swap(user_data);
			return j;
		}

		job* create_child(job* parent, job_function function)
		{
			assert(parent);

			//create child job
			auto jobptr = create(function);

			//assign parent
			jobptr->parent_job = parent;
			//increment parents child count
			++(parent->unfinished_children);

			return jobptr;
		}

		job* create_child(job* parent, job_function function, job_data_ptr user_data)
		{
			auto j = create_child(parent, function);
			assert(user_data);
			j->user_data.swap(user_data);
			return j;
		}

		locked_queue get_queue(thread_id i);

		void run(job* job)
		{
			//add job to this threads queue
			worker_queue* q = nullptr;
			lock_t lock;
			std::tie(q, lock) = get_queue(t_thread_id);

			assert(q);
			assert(lock);
			q->push_front(job);
		}

		bool is_finished(const job*);
		job* find_job();
		void execute(job* job);

		void wait(const job* job)
		{
			assert(job);
			while (!is_finished(job))
			{
				//find another job to run
				auto j = find_job();
				if(j)
					execute(j);
			}
		}

		void finish(job* job)
		{
			//mark job as complete
			const auto children = --(job->unfinished_children);
			if (children == 0)
			{
				if (job->parent_job)
				{
					finish(job->parent_job);
				}
			}
		}

		void clear()
		{
			//the work should have already stopped when this is called.
			assert(g_idle_threads == g_thread_count);
			//clear all the thread queues
			for (thread_id i = 0; i < g_thread_count; ++i)
			{
				lock_t qlk(g_worker_queue_mutexes[i]);
				g_worker_job_queues[i].clear();
			}

			//clear the job list
			lock_t jlk(joblist_mutex);
			g_joblist.clear();
		}

		//==worker functions==
		bool is_ready(const job* job)
		{
			//when children == 1, then only this jobs actual task is left to do.
			return job->unfinished_children == 1;
		}

		bool is_finished(const job* job)
		{
			//if it's 0 or less than one, then the job has been completed
			return job->unfinished_children <= 1;
		}

		//worker init is called once at the start of the program
		void worker_init()
		{
			t_thread_id = g_thread_id_next++;
		}

		locked_queue get_queue(thread_id i)
		{
			assert(i < g_thread_count + 1);
			lock_t lock(g_worker_queue_mutexes[i]);
			return std::make_tuple(&g_worker_job_queues[i], std::move(lock));
		}

		job* find_job()
		{
			job* j = nullptr;

			{
				//grab a job from our queue
				lock_t qlock;
				worker_queue *q = nullptr;
				std::tie(q, qlock) = get_queue(t_thread_id);
				assert(q);
				assert(qlock);

				//out queue works from the front
				if (q->size() > 0)
				{
					j = q->front();
					q->pop_front();
				}
			}

			//or steal one from another queue if ours is empty
			if (!j)
			{
				//TODO: we should be stealing from the longest queue, rather than a random one(what if only one has jobs?)
				// we could try to steal g_thread_count times before finding a job
				auto t = t_thread_id;
				while(t == t_thread_id)
					t = random<thread_id>(0, g_thread_count);

				//stolen queue works from the back
				lock_t qlock;
				worker_queue *q = nullptr;
				std::tie(q, qlock) = get_queue(t);
				assert(q);
				assert(qlock);

				if (q->size() > 0)
				{
					j = q->back();
					q->pop_back();
				}
			}

			if (j)
			{
				//set active
				if (t_sleep)
				{
					t_sleep = false;
					--g_idle_threads;
				}		
			}
			else
			{
				//set inactive
				if (!t_sleep)
				{
					t_sleep = true;
					++g_idle_threads;
				}
				//if we still can't find a job go idle
				std::this_thread::yield();
			}

			return j;
		}

		void ready_wait(const job *job)
		{
			assert(job);
			while (!is_ready(job))
			{
				//find another job to run
				auto j = find_job();
				if (j)
					execute(j);
			}
		}

		void execute(job* job)
		{
			assert(job);
			if (job->unfinished_children > 1)
				ready_wait(job);
			//if true, the job completed properly, if false, it wants another go later
			if (job->function(*job->user_data))
				finish(job);
			else
				run(job);//requeue the job
		}

		void worker_function()
		{
			worker_init();

			while (g_workers_enabled)
			{
				//get job
				auto j = find_job();
				if (j)
					execute(j);
			}
		}	
	}
}