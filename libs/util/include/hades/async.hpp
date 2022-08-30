#ifndef HADES_UTIL_ASYNC_HPP
#define HADES_UTIL_ASYNC_HPP

#include <cassert>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

//#include "hades/random.hpp"

// NOTE: current implementation suffers with async functions starting their own async funcs
//	this results in eating up stack for each additional layer. Something to keep in mind.
//	ei. Dont use async recursively in a heavy manner.

namespace hades
{
	class thread_pool;
}

namespace hades::detail
{
	void set_shared_thread_pool(thread_pool*) noexcept;
	thread_pool* get_shared_thread_pool() noexcept;
}

namespace hades
{
	namespace detail
	{
		template<typename T>
		struct future_data_store
		{
			std::optional<T> value;
		};

		template<>
		struct future_data_store<void>
		{};

		template<typename T>
		struct future_shared_state : future_data_store<T>
		{
			std::exception_ptr e_ptr;
			thread_pool* pool = nullptr;
			std::atomic_bool complete = false;
		};
	}

	template<typename T>
	class future
	{
	public:
		using state_ptr = std::shared_ptr<detail::future_shared_state<T>>;
		future(state_ptr s) : _shared_state{ std::move(s) } {}

		bool ready() noexcept
		{
			assert(_shared_state);
			return std::atomic_load_explicit(&(_shared_state->complete),
				std::memory_order_acquire);
		}

		T get()
		{
			assert(_shared_state);
			while (std::atomic_load_explicit(&(_shared_state->complete),
				std::memory_order_acquire) == false)
			{
				_shared_state->pool->help();
			}

			if (_shared_state->e_ptr)
				std::rethrow_exception(_shared_state->e_ptr);

			if constexpr (!std::is_same_v<T, void>)
			{
				assert(_shared_state->value.has_value());
				return std::move(*(_shared_state->value));
			}
			else
				return;
		}

	private:
		state_ptr _shared_state;
	};

	class thread_pool
	{
	public:
		explicit thread_pool(std::size_t thread_count = std::thread::hardware_concurrency());
		~thread_pool() noexcept; 

		//try and complete one of the queued tasks for the pool
		//can return spuriously without doing any work
		void help()
		{
			auto work = std::function<void()>{};
			{
				static thread_local auto index = 0;
				auto& other_queue = _queues[index++ % size(_queues)];

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

		template<typename Func, typename ...Args> 
		[[nodiscard]]
		auto async(Func&& f, Args&& ...args)
		{
			using R = std::invoke_result_t<Func, Args...>;
			auto shared = std::make_shared<detail::future_shared_state<R>>();
			shared->pool = this;
			auto weak = std::weak_ptr{ shared };

			//wrap function args and call in a lambda
			auto work = [weak, func = std::decay_t<Func>{ std::forward<Func>(f) }, args = std::tuple<std::decay_t<Args>...>(std::move(args)...)]() mutable noexcept {
				auto shared = weak.lock();
				//the future has already been abandoned
				if (!shared)
					return;

				try
				{
					if constexpr(std::is_same_v<R, void>)
						std::apply(func, std::move(args));
					else
						shared->value = std::apply(func, std::move(args));
				}
				catch (...)
				{
					shared->e_ptr = std::current_exception();
				}

				std::atomic_store_explicit(&(shared->complete), true, std::memory_order_release);
				return;
			};

			{
				const auto index = get_worker_thread_id();
				const auto lock = std::scoped_lock{ _queues[index].mut };
				_queues[index].work.emplace_back(false_copyable{ std::move(work) });
				std::atomic_fetch_add_explicit(&_work_count, std::size_t{ 1 }, std::memory_order_relaxed);
			}
			
			//release a thread
			const auto lock = std::scoped_lock{ _condition_mutex };
			_cv.notify_one();
			return future{ std::move(shared) };
		}

		template<typename Func, typename ...Args>
		void detached_async(Func&& f, Args&& ...args) noexcept
		{
			static_assert(std::is_nothrow_invocable_v<Func, Args...>);
			//wrap function args and call in a lambda
			auto work = [func = std::decay_t<Func>{ std::forward<Func>(f) }, args = std::tuple<std::decay_t<Args>...>(std::move(args)...)]() mutable noexcept {
				std::apply(func, std::move(args));
				return;
			};

			{
				const auto index = get_worker_thread_id();
				const auto lock = std::scoped_lock{ _queues[index].mut };
				_queues[index].work.emplace_back(false_copyable{ std::move(work) });
				std::atomic_fetch_add_explicit(&_work_count, std::size_t{ 1 }, std::memory_order_relaxed);
			}

			//release a thread
			const auto lock = std::scoped_lock{ _condition_mutex };
			_cv.notify_one();
			return;
		}

	private:
		//std::function cannot hold non-copyable
		//wrap packaged task in this lying wrapper
		template<typename Task>
		struct false_copyable
		{
			false_copyable() = delete;
			false_copyable(Task t) noexcept : value{ std::move(t) } {}
			false_copyable(const false_copyable&) { throw std::logic_error{ "thread_pool: tried to copy non-copyable function" }; }
			false_copyable(false_copyable&&) noexcept = default;
			void operator()() {
				value->operator()();
				return;
			}
			std::optional<Task> value; // needed to generate correct copy constuctor
		};

		struct thread_work_queue
		{
			std::mutex mut;
			std::deque<std::function<void()>> work;
		};

		static std::size_t get_worker_thread_id() noexcept;

		std::mutex _condition_mutex;
		std::condition_variable _cv;

		using work_queues_t = std::vector<thread_work_queue>;
		work_queues_t _queues;
		std::vector<std::thread> _threads;

		std::atomic_size_t _work_count;
		std::atomic_bool _stop_flag{ false };
	};

	static_assert(!(std::is_copy_constructible_v<thread_pool> || std::is_move_constructible_v<thread_pool> ||
		std::is_copy_assignable_v<thread_pool> || std::is_move_assignable_v<thread_pool>));

	//queue a function instance into the shared thread pool
	template<typename Func, typename ...Args>
	[[nodiscard]]
	auto async(Func&& f, Args&& ...args) -> future<std::invoke_result_t<Func, Args...>>
	{
		auto* pool = detail::get_shared_thread_pool();
		if(pool)
			return pool->async(std::forward<Func>(f), std::forward<Args>(args)...);

		// fall back to syncronous
		using R = std::invoke_result_t<Func, Args...>;
		auto shared = std::make_shared<detail::future_shared_state<R>>();
		if constexpr (std::is_same_v<R, void>)
			std::invoke(std::forward<Func>(f), std::move(args)...);
		else
			shared->value = std::invoke(std::forward<Func>(f), std::move(args)...);

		std::atomic_store_explicit(&shared->complete, true, std::memory_order_relaxed);
		return future{ std::move(shared) };
	}

	template<typename Func, typename ...Args>
	void detached_async(Func&& f, Args&& ...args) noexcept
	{
		static_assert(std::is_nothrow_invocable_v<Func, Args...>);
		auto* pool = detail::get_shared_thread_pool();
		if (pool)
		{
			pool->detached_async(std::forward<Func>(f), std::forward<Args>(args)...);
			return;
		}

		std::invoke(std::forward<Func>(f), std::move(args)...);
		return;
	}
}

#endif //HADES_UTIL_ASYNC_HPP
