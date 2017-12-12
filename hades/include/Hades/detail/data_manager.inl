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
		void data_manager::set(UniqueId key, std::unique_ptr<T> value)
		{
			try
			{
				_resources.set(key, std::move(value));
				//set the values unique id, incase the calling code didn't
				auto v = &**_resources.get_reference<std::unique_ptr<T>>(key);
				v->id = key;
			}
			catch (type_erasure::value_wrong_type &e)
			{
				throw resource_wrong_type(e.what());
			}
		}

		template<class T>
		T* data_manager::get(UniqueId uid)
		{		
			try
			{
				auto value = &**_resources.get_reference<std::unique_ptr<T>>(uid);

				if (!value->loaded)
				{
					refresh(value->id);
					load(value->id);
				}

				return value;
			}
			catch (type_erasure::key_null&)
			{
				//exception -- value no defined
				auto message = "Failed to find resource for unique_id: " + as_string(uid);
				throw resource_null(message.c_str());
			}
			catch (type_erasure::value_wrong_type&)
			{
				//exception -- value is wrong type
				auto message = "Tried to get resource using wrong type, unique_id was : " + as_string(uid);
				throw resource_wrong_type(message.c_str());
			}
		}

		//gets the requested resource, or creates it if not already
		// sets the resources most recent mod to the passed value
		// returns nullptr if unable to return a valid resource
		template<class T>
		T* FindOrCreate(data::UniqueId target, data::UniqueId mod, data::data_manager* data)
		{
			T* r = nullptr;

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
					r = data->get<T>(target);
				}
				catch (data::resource_wrong_type&)
				{
					//name is already used for something else, this cannnot be loaded
					auto modname = data->as_string(mod);
					LOGERROR("Failed to get " + std::string(typeid(T).name()) + " with id: " + data->as_string(target) + ", in mod: " + modname + ", name has already been used for a different resource type.");
				}
			}

			if (r)
				r->mod = mod;

			return r;
		}
	}
}

template<class T>
T yaml_get_scalar(const YAML::Node& node, hades::types::string resource_type, hades::types::string resource_name,
	hades::types::string property_name, hades::data::UniqueId mod, T default_value)
{
	auto value_node = node[property_name];
	if (value_node.IsDefined() && yaml_error(resource_type, resource_name, property_name, "scalar", mod, value_node.IsScalar()))
		return value_node.as<T>(default_value);
	else
		return default_value;
}

template<class T>
std::vector<T> yaml_get_sequence(const YAML::Node& node, hades::types::string resource_type, hades::types::string resource_name,
	hades::types::string property_name, hades::data::UniqueId mod)
{
	std::vector<T> output;
	auto seq = node[property_name];
	if (seq.IsDefined() && yaml_error(resource_type, resource_name, property_name, "sequence", mod, seq.IsSequence()))
	{
		for (auto &i : seq)
			output.push_back(i.as<T>());
	}

	return output;
}