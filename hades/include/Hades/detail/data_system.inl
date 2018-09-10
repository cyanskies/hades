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

			if (target == empty_id)
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
					auto modname = data->get_as_string(mod);
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
		else return data->get_uid(input);
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
	return yaml_get_sequence(node, resource_type, resource_name, property_name, std::vector<T>{}, mod);
}

template<class T, class ConversionFunc>
std::vector<T> yaml_get_sequence(const YAML::Node& node, hades::types::string resource_type, hades::types::string resource_name,
	hades::types::string property_name, ConversionFunc func, hades::unique_id mod)
{
	return yaml_get_sequence(node, resource_type, resource_name, property_name, std::vector<T>{}, func, mod);
}

template<class T>
std::vector<T> yaml_get_sequence(const YAML::Node& node, hades::types::string resource_type, hades::types::string resource_name,
	hades::types::string property_name, const std::vector<T> &previous_sequence, hades::unique_id mod)
{
	return yaml_get_sequence(node, resource_type, resource_name, property_name, previous_sequence, nullptr, mod);
}

namespace
{
	template<typename T, typename ConversionFunction>
	T convert_if(const YAML::Node &n, ConversionFunction convert)
	{
		if constexpr (!std::is_null_pointer_v<ConversionFunction>)
			return convert(n.as<hades::string>());
		else
			return n.as<T>();
	}

	template<typename T>
	void remove_from_sequence(std::vector<T> &container, const T& value)
	{

	}

	template<typename T>
	void add_to_sequence(std::vector<T> &container, const T& value)
	{
		container.emplace_back(value);
	}
}

template<class T, class ConversionFunc>
std::vector<T> yaml_get_sequence(const YAML::Node& node, hades::types::string resource_type, hades::types::string resource_name,
	hades::types::string property_name, const std::vector<T> &previous_sequence, 
	ConversionFunc func, hades::unique_id mod)
{
	//test that func can be used to convert string into T
	if constexpr (!std::is_null_pointer_v<ConversionFunc>)
		static_assert(std::is_constructible< std::function < T(hades::string) >, ConversionFunc >::value,
			"Conversion func must be a function taking hades::string and returning typename T, (hades::string s)->T");

	auto seq = node[property_name];

	if (!seq.IsDefined())
		return previous_sequence;

	std::vector<T> output{ previous_sequence };

	using namespace std::string_view_literals;

	if (seq.IsScalar())
	{
		const auto str{ seq.as<hades::string>() };

		if (str == "="sv)
			output.clear();
		else
			add_to_sequence(output, convert_if<T>(seq, func));
	}
	else if (seq.IsSequence())
	{
		enum class seq_mode {ADD, REMOVE};

		seq_mode mode{ seq_mode::ADD };

		for (const auto &i : seq)
		{
			const auto str{ i.as<hades::string>() };

			if (str == "+"sv)
				mode = seq_mode::ADD;
			else if (str == "-"sv)
				mode = seq_mode::REMOVE;
			else if (str == "="sv)
			{
				mode = seq_mode::ADD;
				output.clear();
			}
			else
			{
				if (mode == seq_mode::ADD)
					add_to_sequence(output, convert_if<T>(i, func));
				else if (mode == seq_mode::REMOVE)
					remove_from_sequence(output, convert_if<T>(i, func));
				else
					assert(false); // ERROR: a new sequence mode has been added and not accounted for?
			}
		}
	}
	else
	{
		yaml_error(resource_type, resource_name, property_name, "sequence", mod, false);
	}

	return output;
}
