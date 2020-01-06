
struct needed_info
{
	//assume these would be ptrs(weak?) to the specific data
	void* ref_to_position;
	void* ref_to_speed;

	//make changes to local copys

	//when done, try to get locks to all the data

	//if failed return false and change nothing

	//otherwise overwrite data and then release locks

	//return true;
};

bool exampleJob(needed_info data)
{
	return true;
}