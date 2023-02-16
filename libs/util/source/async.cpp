#include "hades/async.hpp"

#include <cassert>
#include <iterator>

namespace hades
{
	thread_local static std::size_t worker_id = 0; // default == main thread

	thread_pool::thread_pool(const std::size_t count)
	{
		const auto thread_count = std::max(count, std::size_t{ 1 });

		auto worker_function = [this](const std::size_t thread_id) {
			hades::worker_id = thread_id;
			// keep looping until the pool shuts down
			while (std::atomic_load_explicit(&_stop_flag, std::memory_order_seq_cst) == false)
			{
				// if there is no work, then idle
				if (std::atomic_load_explicit(&_work_count, std::memory_order_acquire) == std::size_t{})
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
						std::atomic_fetch_sub_explicit(&_work_count, std::size_t{ 1 }, std::memory_order_release);
					}
				}

				// steal work
				if (!work)
				{
					// no tasks, lets be a thief
					static thread_local auto index = std::size_t{};
					if (index % size(_queues) == thread_id)
						++index;

					auto& our_queue = _queues[thread_id];
					auto& other_queue = _queues[index++ % size(_queues)];

					//lock both queues
					const auto lock = std::scoped_lock{ our_queue.mut, other_queue.mut };

					//bail if our target has nothing to steal
					const auto count = size(other_queue.work);
					if (count == std::size_t{})
						continue;

					//take half of the queue
					const auto steal_count = (count + 1) / 2;
					auto beg = begin(other_queue.work);
					auto iter = next(beg, /*integer_cast*/static_cast<ptrdiff_t>(steal_count));
					std::move(beg, iter, std::back_inserter(our_queue.work)); // TODO: possible throw?
					other_queue.work.erase(beg, iter);

					work = std::move(our_queue.work.front());
					our_queue.work.pop_front();
					std::atomic_fetch_sub_explicit(&_work_count, std::size_t{ 1 }, std::memory_order_release);
				}

				//if we have a task, then do it
				std::invoke(std::move(work));
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
		_stop_flag.store(true, std::memory_order_relaxed);
		_cv.notify_all();

		for (auto& thread : _threads)
		{
			assert(thread.joinable());
			thread.join();
		}

		return;
	}

	std::size_t thread_pool::get_worker_thread_id() noexcept
	{
		return worker_id;
	}
}

namespace hades::detail
{
	static thread_pool* shared_pool = nullptr;

	void set_shared_thread_pool(thread_pool* p) noexcept
	{
		shared_pool = p;
		return;
	}

	thread_pool* get_shared_thread_pool() noexcept
	{
		return shared_pool;
	}
}
