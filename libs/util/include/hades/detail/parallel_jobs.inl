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
			if constexpr(std::is_invocable_r_v<bool, Func, job_system&, JobData>)
			{
				//auto jj = std::ref(j);
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
	inline job_system::job* job_system::create(Func func,
		JobData data)
	{
		//TODO: some static asserts here and in create_child
		//create a new job and store it in the global jobstore
		auto job_ptr = create();
		job_ptr->function = detail::create_job_invoker(*this, func, data);
		return job_ptr;
	}

	template<typename Func, typename JobData>
	inline job_system::job* job_system::create_child(job_system::job* parent,
		Func func, JobData data)
	{
		auto j = create_child(parent);
		j->function = detail::create_job_invoker(*this, func, data);
		return j;
	}
}