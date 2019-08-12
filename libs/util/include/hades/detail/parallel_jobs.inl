#include "hades/parallel_jobs.hpp"

#include <cassert>
#include <type_traits>

#include "hades/types.hpp"

namespace hades
{
	namespace detail
	{
		template<typename Func, typename JobData>
		job_function create_job_invoker(job_system &j, Func f, JobData d)
		{
			if constexpr(std::is_invocable_r_v<bool, Func, job_system&, JobData>)
			{
				return [j = std::ref(j), f, d]()->bool {
					return std::invoke(f, j, d);
				};
			}
			else if constexpr (std::is_invocable_v<Func, job_system&, JobData>)
			{ //correct parameters, but does't return bool
				return [j = std::ref(j), f, d]()->bool {
					std::invoke(f, j, d);
					return true;
				};
			}
			else if constexpr(std::is_invocable_r_v<bool, Func, JobData>)
			{ 
				return [f, d]()->bool {
					return std::invoke(f, d);
				};
			}
			else if constexpr (std::is_invocable_r_v<bool, Func, JobData>)
			{ //correct parameters without the system reference, but doesn't return bool
				return [f, d]()->bool {
					std::invoke(f, d);
					return true;
				};
			}
			else
				static_assert(always_false<Func, JobData>::value, "Function passed to job must accept JobData as parameter, or job_system& and JobData as parameters");
		}
	}

	template<typename Func, typename JobData>
	inline job* job_system::create(Func func,
		JobData data)
	{
		return _create(detail::create_job_invoker(*this, func, data));
	}

	template<typename Func, typename JobData>
	inline job* job_system::create_child(job* parent,
		Func func, JobData data)
	{
		return _create_child(parent, detail::create_job_invoker(*this, func, data));
	}

	template<typename Func, typename JobData>
	inline job* job_system::create_rchild(job* rparent, Func func, JobData data)
	{
		return _create_rchild(rparent, detail::create_job_invoker(*this, func, data));
	}

	template<typename Func, typename JobData>
	inline job* job_system::create_child_rchild(job* parent, job* rparent, Func f, JobData d)
	{
		return _create_child_rchild(parent, rparent, detail::create_job_invoker(*this, f, d));
	}

	template<typename InputIt>
	inline void job_system::run(InputIt first, InputIt last)
	{
		static_assert(std::is_same_v<std::iterator_traits<InputIt>::value_type, job*>);
		const auto id = _thread_id();
		auto [queue, lock] = _get_queue(id);
		std::ignore = lock;
		assert(queue);
		queue->insert(std::begin(*queue), first, last);
		return;
	}
}