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
		template<typename T>
		bool operator<(const UniqueId_t<T>& lhs, const UniqueId_t<T>& rhs)
		{
			return lhs._value < rhs._value;
		}

		template<class T>
		void data_manager::set(UniqueId key, std::unique_ptr<T> value)
		{
			try
			{
				_resources.set(key, std::move(value));
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
				return &**_resources.get_reference<std::unique_ptr<T>>(uid);
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
	}
}