#include "hades/parallel_jobs.hpp"

#include <cassert>
#include <type_traits>

#include "hades/types.hpp"

namespace hades
{
	namespace detail
	{
		template<typename Func, typename JobData>
		job_system::job_function create_job_invoker(job_system &j, Func f, JobData d)
		{
			if constexpr(std::is_constructible<std::function<bool(job_system&, JobData)>, Func >::value)
			{
				return [j, f, d]()->bool {
					return std::invoke(f, j, d);
				};
			}
			else if constexpr(std::is_constructible<std::function<bool(JobData)>, Func >::value)
			{
				return [f, d]()->bool {
					return std::invoke(f, d);
				};
			}
			else
				static_assert(always_false<Func, JobData>::value, "Function passed to job but accept JobData as parameter, or job_system& and JobData as parameters");
		}
	}

	template<typename Func, typename JobData>
	inline job_system::job* job_system::create(Func func,
		JobData data)
	{
		assert(func);

		//create a new job and store it in the global jobstore
		lock_t guard{ _jobs_mutex };
		_jobs.emplace_back();
		job* job_ptr = &_jobs.back();
		job_ptr->function = detail::create_job_invoker(this, func, data);
		return job_ptr;
	}

	template<typename Func, typename JobData>
	inline job_system::job* job_system::create_child(job_system::job* parent,
		Func func, JobData data)
	{
		auto j = create(func, data);
		j->parent_job = parent;
		//increment parents child count
		++(parent->unfinished_children);
		return j;
	}
}