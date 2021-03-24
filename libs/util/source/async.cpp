#include "hades/async.hpp"

#include <cassert>

namespace hades
{
	thread_pool::thread_pool(std::size_t thread_count)
	{
		thread_count = std::max(thread_count, std::size_t{ 1 });

		// noexcept: if this throws the std::thread will call terminate anyway
		auto worker_function = [this](const std::size_t thread_id) noexcept {
			// keep looping until the pool shuts down
			while (std::atomic_load_explicit(&_stop_flag, std::memory_order_relaxed) == false)
			{
				// if their is no work, then idle
				if (std::atomic_load_explicit(&_work_count, std::memory_order_relaxed) == std::size_t{})
				{
					auto lock = std::unique_lock{ _condition_mutex };
					_cv.wait(lock);
					continue;
				}

				//find some work
				auto work = std::function<void()>{};
				{
					auto& queue = _queues[thread_id];
					const auto lock = std::scoped_lock{ queue.mut };
					if (!std::empty(queue.work))
					{
						work = std::move(queue.work.front());
						queue.work.pop_front();
						std::atomic_fetch_sub_explicit(&_work_count, std::size_t{ 1 }, std::memory_order_relaxed);
					}
				}

				// steal work
				if (!work)
				{
					// no tasks, lets be a thief
					// pick from a pool of queues one less than actually exist
					auto index = random(std::size_t{}, std::size(_queues) - 2);
					if (index == thread_id)
						++index;

					auto& our_queue = _queues[thread_id];
					auto& other_queue = _queues[index];

					//lock both queues
					const auto lock = std::scoped_lock{ our_queue.mut, other_queue.mut };

					//bail if our target has nothing to steal
					const auto count = size(other_queue.work);
					if (count == std::size_t{})
						continue;

					//take half of the queue
					const auto steal_count = static_cast<std::size_t>(std::ceilf(count / 2.f));
					auto beg = begin(other_queue.work);
					auto iter = next(beg, steal_count);
					std::move(beg, iter, std::back_inserter(our_queue.work)); // TODO: possible throw?
					other_queue.work.erase(beg, iter);

					work = std::move(our_queue.work.front());
					our_queue.work.pop_front();
					std::atomic_fetch_sub_explicit(&_work_count, std::size_t{ 1 }, std::memory_order_relaxed);
				}

				//if we have a task, then do it
				std::invoke(work);
			}

			return;
		};

		_threads.reserve(thread_count);
		_queues = work_queues_t{ thread_count };
		for (auto i = std::size_t{}; i < thread_count; ++i)
			_threads.emplace_back(worker_function, i);

		return;
	}

	thread_pool::~thread_pool() noexcept
	{
		_stop_flag.store(true, std::memory_order::memory_order_relaxed);
		_cv.notify_all();

		for (auto& thread : _threads)
		{
			assert(thread.joinable());
			thread.join();
		}

		return;
	}

	void thread_pool::help()
	{
		auto work = std::function<void()>{};
		{
			const auto index = random(std::size_t{}, std::size(_queues) - 1);
			auto& other_queue = _queues[index];

			const auto lock = std::scoped_lock{ other_queue.mut };

			//bail if our target has nothing to steal
			if (empty(other_queue.work))
				return;

			work = std::move(other_queue.work.front());
			other_queue.work.pop_front();
			std::atomic_fetch_sub_explicit(&_work_count, std::size_t{ 1 }, std::memory_order_relaxed);
		}
		std::invoke(work);
		return;
	}
}

namespace hades::detail
{
	static thread_pool* shared_pool = nullptr;

	void hades::detail::set_shared_thread_pool(thread_pool* p) noexcept
	{
		shared_pool = p;
		return;
	}

	thread_pool* hades::detail::get_shared_thread_pool() noexcept
	{
		return shared_pool;
	}
}
