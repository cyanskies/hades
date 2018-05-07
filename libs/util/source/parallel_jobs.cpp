#include "hades/parallel_jobs.hpp"

#include <any>
#include <cassert>
#include <utility>

namespace hades
{
	//implementation based on article series
	//https://blog.molecular-matters.com/2015/08/24/job-system-2-0-lock-free-work-stealing-part-1-basics/

	namespace detail
	{
		thread_local void* t_current_job = nullptr;
		thread_local thread_id t_thread_id;
		constexpr auto t_main_thread_id = std::numeric_limits<thread_id>::max();
	}

	//
	job_system::job_system() : job_system(-1)
	{}

	job_system::job_system(types::int32 threads)
	{
		_init(threads);
		_create_threads();
	}

	job_system::~job_system()
	{
		_join.store(true);
		_condition.notify_all();

		for (auto &t : _thread_pool)
		{
			assert(t.joinable());
			t.detach();
		}
	}

	bool job_system::ready() const
	{
		std::unique_lock<std::mutex> cv_lock(_condition_mutex, std::try_to_lock);
		lock_t job_lock(_jobs_mutex);
		return _jobs.empty() && cv_lock.owns_lock();
	}

	void job_system::run(job_system::job* j)
	{
		const auto id = _thread_id();
		auto &[queue, lock] = _get_queue(id);
		std::ignore = lock;
		assert(queue);
		queue->push_front(j);
	}

	void job_system::wait(job_system::job* job)
	{
		assert(job);
		assert(ready());
		_condition.notify_all();

		while (!_is_finished(job))
		{
			//find another job to run
			auto j = _find_job();
			if (j)
				_execute(j);
		}
	}

	void job_system::clear()
	{
		//the work should have already stopped when this is called.
		assert(ready());
		//clear all the thread queues
		for (std::size_t i = 0; i < _thread_count; ++i)
		{
			lock_t qlk(_worker_queues_mutex[i]);
			_worker_queues[i].clear();
		}

		//clear the job list
		lock_t jlk(_jobs_mutex);
		_jobs.clear();
	}

	void job_system::_init(types::int32 threads)
	{
		if (threads > 0)
			_thread_count = threads - 1;
		else
			_thread_count = std::thread::hardware_concurrency() - 1;
		
		_thread_pool = thread_list{ _thread_count };
		_worker_queues = worker_queue_list{ _thread_count + 1 };
		_worker_queues_mutex = worker_mutex_list{ _thread_count + 1 };
	}

	void job_system::_create_threads()
	{
		thread_id next_id = 0;

		for (auto &t : _thread_pool)
			t = std::thread{ [this](thread_id id){ _worker_function(id); } , next_id++ };

		_worker_init(next_id);
	}

	job_system::thread_id job_system::_thread_id() const
	{
		if (detail::t_thread_id == detail::t_main_thread_id)
			return _main_thread_id();
		else
			return detail::t_thread_id;
	}

	void job_system::_worker_init(thread_id id)
	{
		detail::t_thread_id = id;
	}

	void job_system::_worker_function(thread_id id)
	{
		_worker_init(id);

		while (_join.load())
		{
			//get job
			auto j = _find_job();
			if (j)
				_execute(j);
			else
			{
				std::unique_lock<std::mutex> lock(_condition_mutex);
				_condition.wait(lock);
			}
		}
	}

	bool job_system::_is_ready(const job *j) const
	{
		//when children == 1, then only this jobs actual task is left to do.
		return j->unfinished_children == 1;
	}

	bool job_system::_is_finished(const job *j) const
	{
		//if it's 0 or less than one, then the job has been completed
		return j->unfinished_children <= 1;
	}

	void job_system::_finish(job *j)
	{
		assert(j);
		//mark job as complete
		const auto c = --(j->unfinished_children);
		if (c == 0)
		{
			if (j->parent_job)
				_finish(j->parent_job);
		}
	}

	typename job_system::locked_queue job_system::_get_queue(thread_id i)
	{
		assert(i < _thread_count + 1);
		queue_lock lock(_worker_queues_mutex[i]);
		worker_queue *q = &_worker_queues[i];
		return { q, std::move(lock) };
	}

	job_system::job* job_system::_find_job()
	{
		job* j = nullptr;
		const auto id = _thread_id();
		{
			//grab a job from our queue
			auto [q, qlock] = _get_queue(id);
			assert(q);
			std::ignore = qlock;
			
			//out queue works from the front
			if (q->size() > 0)
			{
				j = q->front();
				q->pop_front();
			}
		}

		if (!j)
			j = _steal_job(id);

		return j;
	}

	job_system::job *job_system::_steal_job(thread_id id)
	{
		//find the queue with the most jobs, and take half of them into our queue
		//we know our queue must be empty

		for (std::size_t i = 0; i < _worker_queues.size(); ++i)
			;

		return nullptr;
	}

	void job_system::_ready_wait(job *j)
	{
		assert(j);
		while (!_is_ready(j))
		{
			//find another job to run
			auto j = _find_job();
			if (j)
				_execute(j);
		}
	}

	void job_system::_execute(job *j)
	{
		assert(j);
		if (j->unfinished_children > 1)
			_ready_wait(j);

		if (j->function)
		{
			//if true, the job completed properly, if false, it wants another go later
			if (j->function())
				_finish(j);
			else
				run(j);//requeue the job
		}
	}

	job_system::thread_id job_system::_main_thread_id() const
	{
		return _thread_count;
	}
}