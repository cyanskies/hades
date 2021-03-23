#ifndef HADES_UTIL_ASYNC_HPP
#define HADES_UTIL_ASYNC_HPP

#include <atomic>
#include <cassert>
#include <deque>
#include <functional>
#include <future>
#include <optional>
#include <type_traits>

#include "hades/utility.hpp" // for hades::random

namespace hades
{
	class thread_pool
	{
	public:
		explicit thread_pool(std::size_t thread_count = std::thread::hardware_concurrency());
		~thread_pool() noexcept; 

		template<typename Func, typename ...Args> 
		[[nodiscard]]
		auto async(Func&& f, Args&& ...args)
		{
			//wrap function args and call in a lambda
			auto func = [func = std::forward<Func>(f), args = std::forward_as_tuple(std::forward<Args>(args)...)]() mutable {
				return std::apply(func, std::move(args));
			};

			//wrap the no param function in a packaged task
			auto task = std::packaged_task{ std::move(func) };
			auto future = task.get_future();

			//wrap the task in a no return lambda
			auto work = false_copyable{ std::move(task) };// [func = std::move(task)] () mutable noexcept -> void {
				/*if (func.valid())
					func();
				return;
			};*/

			//stick it in a random queue
			const auto index = random(std::size_t{}, std::size(_queues) - 1);
			{
				const auto lock = std::scoped_lock{ _queues[index].mut };
				_queues[index].work.emplace_back(std::move(work));
				std::atomic_fetch_add(&_work_count, std::size_t{ 1 });
			}
			
			//release a thread
			const auto lock = std::scoped_lock{ _condition_mutex };
			_cv.notify_one();
			return future;
		}

	private:
		//std::function cannot hold non-copyable
		//wrap packaged task in this lying wrapper
		template<typename Task>
		struct false_copyable
		{
			false_copyable() = delete;
			false_copyable(Task&& t) noexcept : value{ std::forward<Task>(t) } {}
			false_copyable(const false_copyable&) { throw std::logic_error{ "thread_pool: tried to copy non-copyable function" }; }
			false_copyable(false_copyable&&) noexcept = default;
			void operator()() {
				if(value->valid())
					value->operator()();
				return;
			}
			std::optional<Task> value;
		};

		struct thread_work_queue
		{
			std::mutex mut;
			std::deque<std::function<void()>> work;
		};

		std::mutex _condition_mutex;
		std::condition_variable _cv;

		using work_queues_t = std::vector<thread_work_queue>;
		work_queues_t _queues;
		std::vector<std::thread> _threads;

		std::atomic_size_t _work_count;
		std::atomic_bool _stop_flag{ false };
	};

	static_assert(!(std::is_copy_constructible_v<thread_pool> || std::is_nothrow_move_constructible_v<thread_pool> ||
		std::is_copy_assignable_v<thread_pool> || std::is_nothrow_move_assignable_v<thread_pool>));

	namespace detail
	{
		void set_shared_thread_pool(thread_pool*) noexcept;
		thread_pool* get_shared_thread_pool() noexcept;
	}

	//queue a function instance into the shared thread pool
	template<typename Func, typename ...Args>
	[[nodiscard]]
	auto async(Func&& f, Args&& ...args)
	{
		auto* pool = detail::get_shared_thread_pool();
		if(pool)
			return pool->async(std::forward<Func>(f), std::forward<Args>(args)...);

		// if there's no shared pool then just use std async
		// will probably end up as a deferred call
		return std::async(std::forward<Func>(f), std::forward<Args>(args)...);
	}
}

#endif //HADES_UTIL_ASYNC_HPP
