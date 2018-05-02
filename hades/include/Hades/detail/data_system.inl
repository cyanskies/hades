#include "Hades/Data.hpp"
#include "Hades/Logging.hpp"

namespace hades
{
	namespace resources
	{
		template<typename T>
		void resource_type<T>::load(data::data_manager* data)
		{
			if (_resourceLoader)
				_resourceLoader(this, data);
		}
	}

	namespace data
	{
		template<class T>
		T* FindOrCreate(unique_id target, unique_id mod, data_manager *data)
		{
			T* r = nullptr;

			if (target == EmptyId)
			{
				LOGERROR("Tried to create resource with hades::EmptyId, this id is reserved for unset Unique Id's, resource type was: " + types::string(typeid(T).name()));
				return r;
			}
			
			if (!data->exists(target))
			{
				auto new_ptr = std::make_unique<T>();
				r = &*new_ptr;
				data->set<T>(target, std::move(new_ptr));

				r->id = target;
			}
			else
			{
				try
				{
					r = data->get<T>(target, data_manager::no_load);
				}
				catch (data::resource_wrong_type &e)
				{
					//name is already used for something else, this cannnot be loaded
					auto modname = data->getAsString(mod);
					LOGERROR(types::string(e.what()) + "In mod: " + modname + ", name has already been used for a different resource type.");
				}
			}

			if (r)
				r->mod = mod;

			return r;
		}
	}
}

template<class T, class Iter>
std::vector<const T*> convert_string_to_resource(Iter first, Iter last, hades::data::data_manager *data)
{
	//convert strings into ids
	std::vector<hades::unique_id> ids;
	std::transform(first, last, std::back_inserter(ids),
		[data](hades::types::string input) {
		if (input.empty()) return hades::unique_id::zero;
		else return data->getUid(input);
	});

	//get objects
	std::vector<const T*> objs;
	std::transform(std::begin(ids), std::end(ids), std::back_inserter(objs),
		[data](hades::unique_id id)->const T*{
			if (id == hades::unique_id::zero)
			return nullptr;
			else
				return data->get<T>(id);
		});

	//remove any null_ptrs
	objs.erase(std::remove_if(std::begin(objs), std::end(objs),
		[](const T* a) {
		return a == nullptr;
	}), std::end(objs));

	return objs;
}

template<class T>
T yaml_get_scalar(const YAML::Node& node, hades::types::string resource_type, hades::types::string resource_name,
	hades::types::string property_name, hades::unique_id mod, T default_value)
{
	auto value_node = node[property_name];
	if (value_node.IsDefined() && yaml_error(resource_type, resource_name, property_name, "scalar", mod, value_node.IsScalar()))
		return value_node.as<T>(default_value);
	else
		return default_value;
}

template<class T>
std::vector<T> yaml_get_sequence(const YAML::Node& node, hades::types::string resource_type, hades::types::string resource_name,
	hades::types::string property_name, hades::unique_id mod)
{
	std::vector<T> output;
	auto seq = node[property_name];

	if(seq.IsDefined())
	{
        if (seq.IsSequence())
        {
            for (const auto &i : seq)
                output.push_back(i.as<T>());
        }
        else if (seq.IsScalar())
        {
            output.push_back(seq.as<T>(T()));
        }
    }

	return output;
}
