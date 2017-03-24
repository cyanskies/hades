#include "Hades/parallel_jobs.hpp"
#include "Hades/transactional_map.hpp"

struct needed_info : public hades::parallel_jobs::job_data
{
	//assume these would be ptrs(weak?) to the specific 
	void* ref_to_position;
	void* ref_to_speed;
};

bool exampleJob(hades::parallel_jobs::job_data* data)
{
	//convert job data to our actual job information type
	auto info = static_cast<needed_info*>(data);

	return true;
}