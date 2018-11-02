#ifndef HADES_UTIL_PARALLELJOBS_HPP
#define HADES_UTIL_PARALLELJOBS_HPP

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>

#include <hades/types.hpp>

namespace hades 
{
	namespace detail
	{
		using thread_id = std::size_t;
	}

	class job_system
	{
	public:
		using job_function = std::function<bool(void)>;

		struct job
		{
			job() = default;
			job(const job&);
			job& operator=(const job&);
			
			job_function function;
			job* parent_job = nullptr;
			std::atomic<types::int32> unfinished_children = 1;
		};

		//choses the number of threads based on hardware support
		job_system();
		//if threads is positive, then that many threads will be used when wait is called
		//otherwise the number of threads is chosen based on hardware support
		job_system(types::int32 threads);
		~job_system();

		bool ready() const;

		job* create();
		template<typename Func, typename JobData>
		job* create(Func, JobData);
		template<typename Func, typename JobData>
		job* create_child(job* parent, Func, JobData);

		void run(job*);
		void wait(job*);
		void clear();

	private:
		using worker_queue = std::deque<job*>;
		using lock_t = std::lock_guard<std::mutex>;
		using queue_lock = std::unique_lock<std::mutex>;
		using locked_queue = std::tuple<worker_queue*, queue_lock>;
		using thread_id = detail::thread_id;

		void _init(types::int32);
		void _create_threads();
		thread_id _thread_id() const;
		void _worker_init(thread_id);
		void _worker_function(thread_id);
		bool _is_ready(const job*) const;
		bool _is_finished(const job*) const;
		void _finish(job*);
		locked_queue _get_queue(thread_id);
		job* _find_job();
		job* _steal_job(thread_id);
		void _ready_wait(job*);
		void _execute(job*);
		bool _threads_ready() const;

		thread_id _main_thread_id() const;
		job* _main_current_job = nullptr;

		std::size_t _thread_count = 0;
		std::atomic_bool _join = false;
		using thread_list = std::vector<std::thread>;
		thread_list _thread_pool;
		using worker_queue_list = std::vector<worker_queue>;
		worker_queue_list _worker_queues;
		using worker_mutex_list = std::vector<std::mutex>;
		mutable worker_mutex_list _worker_queues_mutex;
		std::vector<std::unique_ptr<job>> _jobs;
		mutable std::mutex _jobs_mutex;
		
		std::condition_variable _condition;
		mutable std::mutex _condition_mutex;
	};
}

#include "hades/detail/parallel_jobs.inl"

#endif // !HADES_UTIL_PARALLELJOBS_HPP
