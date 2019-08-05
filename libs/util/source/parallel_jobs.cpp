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

	struct job
	{
		job() noexcept = default;
		job(const job&);
		job& operator=(const job&);

		job_function function;
		job* parent_job = nullptr;
		job* rparent_job = nullptr;
		std::atomic<types::int32> unfinished_children = 1;
	};

	job::job(const job &other)
		: function(other.function), parent_job(other.parent_job), 
		rparent_job(other.rparent_job), unfinished_children(other.unfinished_children.load())
	{}

	job& job::operator=(const job &other)
	{
		job j{ other };
		std::swap(*this, j);
		return *this;
	}

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

	job* job_system::create()
	{
		lock_t guard{ _jobs_mutex };
		_jobs.emplace_back(std::make_unique<job>());
		return &*_jobs.back();
	}

	job* job_system::create_child(job* parent)
	{
		auto j = create();
		j->parent_job = parent;
		//increment parents child count
		++(parent->unfinished_children);
		return j;
	}

	job* job_system::create_rchild(job* r)
	{
		auto j = create();
		j->rparent_job = r;
		return j;
	}

	job* job_system::create_child_rchild(job* parent, job* rparent)
	{
		auto j = create_child(parent);
		j->rparent_job = rparent;
		return j;
	}

	void job_system::run(job* j)
	{
		const auto id = _thread_id();
		auto [queue, lock] = _get_queue(id);
		std::ignore = lock;
		assert(queue);
		queue->push_front(j);
	}

	static bool is_finished(const job* j)
	{
		//if it's 0 or less than one, then the job has been completed
		return j->unfinished_children <= 1;
	}

	void job_system::wait(job* job)
	{
		assert(job);
		_condition.notify_all();

		while (!is_finished(job))
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
		assert(_threads_ready());
		//clear all the thread queues
		for (std::size_t i = 0; i < _thread_count; ++i)
		{
			const auto lock = std::scoped_lock{ _worker_queues_mutex[i] };
			_worker_queues[i].clear();
		}

		//clear the job list
		lock_t jlk(_jobs_mutex);
		_jobs.clear();
	}

	void job_system::change_thread_count(int32 threads)
	{
		_join.store(true);
		_condition.notify_all();

		for (auto& t : _thread_pool)
		{
			assert(t.joinable());
			t.detach();
		}

		_join = false;
		_main_current_job = nullptr;

		if (threads == 0)
			threads = 1;

		_init(threads);
		_create_threads();
	}

	std::size_t job_system::get_thread_count() const noexcept
	{
		return _thread_count + 1;
	}

	void job_system::_init(types::int32 threads)
	{
		if (threads > 0)
			_thread_count = static_cast<std::size_t>(threads) - 1;
		else if (std::thread::hardware_concurrency() == 0)
			_thread_count = 0;
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

	static bool is_ready(const job *j)
	{
		if (j->rparent_job)
			return j->unfinished_children == 1 &&
			is_finished(j->rparent_job);
		//when children == 1, then only this jobs actual task is left to do.
		return j->unfinished_children == 1;
	}

	static void finish(job *j)
	{
		assert(j);
		//mark job as complete
		const auto c = --(j->unfinished_children);
		if (c == 0)
		{
			if (j->parent_job)
				finish(j->parent_job);
		}
	}

	typename job_system::locked_queue job_system::_get_queue(thread_id i)
	{
		assert(i < _thread_count + 1);
		queue_lock lock(_worker_queues_mutex[i]);
		worker_queue *q = &_worker_queues[i];
		return { q, std::move(lock) };
	}

	job* job_system::_find_job()
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

	job *job_system::_steal_job(thread_id id)
	{
		//find the queue with the most jobs, and take half of them into our queue
		//we know our queue must be empty
		std::size_t max = id, max_count = 0;

		for (std::size_t i = 0; i < _thread_count; ++i)
		{
			if (i == id)
				continue; //our thread is already empty

			const auto[queue, lock] = _get_queue(i);
			std::ignore = lock;

			if (queue->size() > max_count)
				max = i;
		}

		if (max == id) // there aren't any jobs left to steal.
			return nullptr;

		auto[queue, lock] = _get_queue(max);
		std::ignore = lock;

		//split this queue in half
		const auto distance = std::distance(std::begin(*queue), std::end(*queue));
		const auto mid_point = std::begin(*queue) + distance / 2;

		auto[my_queue, my_lock] = _get_queue(id);
		std::ignore = my_lock;

		const auto last_moved = std::move(std::begin(*queue), mid_point, std::end(*my_queue));

		queue->erase(std::begin(*queue), last_moved);

		if (my_queue->empty())
			return nullptr;

		auto j = my_queue->front();
		my_queue->pop_front();

		return j;
	}

	void job_system::_ready_wait(job *j)
	{
		assert(j);
		while (!is_ready(j))
		{
			//find another job to run
			auto j2 = _find_job();
			if (j2)
				_execute(j2);
		}
	}

	void job_system::_execute(job *j)
	{
		assert(j);
		if (!is_ready(j))
			_ready_wait(j);

		if (j->function)
		{
			detail::t_current_job = j;
			//if true, the job completed properly, if false, it wants another go later
			if (j->function())
				finish(j);
			else
				run(j);//requeue the job

			detail::t_current_job = nullptr;
		}
	}

	job_system::thread_id job_system::_main_thread_id() const
	{
		return _thread_count;
	}


	bool job_system::_threads_ready() const
	{
		std::unique_lock<std::mutex> cv_lock(_condition_mutex, std::try_to_lock);
		return cv_lock.owns_lock();
	}

	job* job_system::_create(job_function f)
	{
		auto job_ptr = create();
		job_ptr->function = std::move(f);
		return job_ptr;
	}

	job* job_system::_create_child(job *p, job_function f)
	{
		auto j = create_child(p);
		j->function = std::move(f);
		return j;
	}

	job* job_system::_create_rchild(job *p, job_function f)
	{
		auto j = create_rchild(p);
		j->function = std::move(f);
		return j;
	}

	job* job_system::_create_child_rchild(job *p, job *rp, job_function f)
	{
		auto j = create_child_rchild(p, rp);
		j->function = std::move(f);
		return j;
	}

	job* get_parent(job *j)
	{
		assert(j);
		return j->parent_job;
	}
}