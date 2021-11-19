#ifndef HADES_SAVE_LOAD_API_HPP
#define HADES_SAVE_LOAD_API_HPP

namespace hades
{
	struct level_save;
	struct object_save_data;
	struct object_save_instance;
}

namespace hades::game::level
{
	//load everything, this is the basic level script behaviour
	void load(const level_save&);
	//load all objects
	void load(const object_save_data&);
	//load the passed object
	void load(const object_save_instance&);
}

#endif //!HADES_SAVE_LOAD_API_HPP
