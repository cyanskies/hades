#ifndef HADES_DATA_HPP
#define HADES_DATA_HPP

#include <functional>
#include <map>
#include <shared_mutex>
#include <tuple>

#include "hades/resource_base.hpp"
#include "hades/string.hpp"
#include "hades/uniqueid.hpp"

//Data provides thread safe access to the hades data manager.
namespace hades
{
	namespace data
	{
		class data_manager;
		class parser_node;
	}

	namespace resources
	{
		using parser_func = std::function<void(unique_id mod, const data::parser_node&, data::data_manager&)>;

		class resource_link_base
		{
		public:
			constexpr explicit resource_link_base(unique_id id) noexcept : _id{ id } {}

			virtual ~resource_link_base() noexcept = default;

			virtual void update_link() = 0;
			unique_id id() const noexcept
			{
				return _id;
			};

			virtual operator bool() const noexcept = 0;

		private:
			unique_id _id = unique_zero;
		};

		// resources use these to point to one another
		// this lets the ptrs be updated from the data manager
		template<typename T>
		class resource_link_type : public resource_link_base
		{
		public:
			using get_func = const T* (*)(unique_id);

			constexpr explicit resource_link_type(unique_id id, get_func get) noexcept :
				resource_link_base{ id }, _get{ get } {}

			void update_link() override;

			const T& operator*() const
			{
				return *_resource;
			}

			const T* operator->() const
			{
				return _resource;
			}

			operator bool() const noexcept override
			{
				return _resource;
			}

		private:
			const T* _resource = nullptr;
			get_func _get;

		};

		template<typename T>
		class resource_link
		{
		public:
			constexpr resource_link() noexcept = default;
			constexpr resource_link(resource_link_type<T>* link) noexcept : _link{ link } {}

			unique_id id() const noexcept
			{
				return (*_link).id();
			}

			const T* get() const noexcept
			{
				return &**_link;
			}

			const T& operator*() const
			{
				return **_link;
			}

			const resource_link_type<T>& operator->() const
			{
				return *_link;
			}

			operator bool() const noexcept
			{
				return _link && *_link;
			}

		private:
			const resource_link_type<T>* _link = nullptr;
		};

		template<typename T>
		std::vector<unique_id> get_ids(const std::vector<resource_link<T>>&);
	}

	namespace data
	{
		//tag type to request that a resource not be lazy loaded
		struct no_load_t {};
		constexpr no_load_t no_load{};

		template<class T>
		const T* get(unique_id id);

		class data_manager
		{
		public:
			virtual ~data_manager() = default;

			virtual void register_resource_type(std::string_view name, resources::parser_func parser) = 0;

			//returns a resource if it exists, or creates it if it doesn't
			//this should be used when writing parsers
			//allows creating a ptr to a resource that hasn't actually been defined yet
			// returns nullptr on error
			template<class T>
			[[nodiscard]] T* find_or_create(unique_id target, unique_id mod, std::string_view group = {});

			template<typename T>
			[[nodiscard]] std::vector<T*> find_or_create(const std::vector<unique_id>& target, unique_id mod, std::string_view group = {});

			//returns true if the id has a resource associated with it
			bool exists(unique_id id) const;

			template<typename T>
			resources::resource_link<T> make_resource_link(unique_id, typename resources::resource_link_type<T>::get_func = data::get<T>);
			template<typename T>
			std::vector<resources::resource_link<T>> make_resource_link(const std::vector<unique_id>&, typename resources::resource_link_type<T>::get_func = data::get<T>);

			//returns the resource associated with the id
			//returns the resource base class
			//throws resource_null if the id doesn't refer to a resource
			resources::resource_base *get_resource(unique_id id);
			//as above, returns null if the id doesn't refer to a resource
			resources::resource_base *try_get_resource(unique_id id) noexcept;
			//gets a non-owning ptr to the resource represented by id
			//if the reasource has not been loaded it will be loaded before returning
			// throws resource_null or resource_wrong_type
			template<class T>
			T *get(unique_id id);

			//gets a non-owning ptr to the resource represented by id
			// throws resource_null or resource_wrong_type
			template<class T>
			T *get(unique_id id, const no_load_t);

			enum class get_error {
				ok,
				no_resource_for_id,
				resource_wrong_type
			};

			template<class T>
			struct try_get_return
			{
				//try_get_return(T* t) : result(t) {}
				//try_get_return(get_error e) : error(e) {}

				T *result = nullptr;
				get_error error = get_error::ok;
			};

			//gets a non-owning ptr to the resource represented by id
			//if the reasource has not been loaded it will be loaded before returning
			template<class T>
            try_get_return<T> try_get(unique_id id) noexcept;

			//gets a non-owning ptr to the resource represented by id
			template<class T>
            try_get_return<T> try_get(unique_id id, const no_load_t) noexcept;

			//creates a resource with the value of ptr
			//and assigns it to the name id
			//throws resource_name_already_used if the id already refers to a resource
			template<class T>
			void set(unique_id id, std::unique_ptr<T> ptr, std::string_view group = {});

			void update_all_links();

			inline void finalise_main_parse() noexcept
			{
				_main_loaded = true;
			}

			//refresh functions request that the mentioned resources be pre-loaded
			virtual void refresh() = 0;
			virtual void refresh(unique_id) = 0;
			template<class First, class Last>
			void refresh(First first, Last last);

			virtual string get_as_string(unique_id id) const = 0;
			virtual unique_id get_uid(std::string_view name) const = 0;
			virtual unique_id get_uid(std::string_view name) = 0;

			using resource_group = std::pair<string, std::vector<const resources::resource_base*>>;

		protected:
			bool _main_loaded = false;

		private:
			struct resource_storage
			{
				std::vector<std::unique_ptr<resources::resource_base>> resources;
				std::vector<resource_group> resources_by_type;
			};

			// set to true after the main mod has been parsed
			// causes additional resources to be placed in the leaf
			
			resource_storage _main;
			resource_storage _leaf;
			std::vector<std::unique_ptr<resources::resource_link_base>> _resource_links;
			//std::vector<mod> _mods;
			//std::vector<std::unique_ptr<resources::resource_base>> _resources2;
			//std::map<unique_id, std::unique_ptr<resources::resource_base>> _resources;
		};

		namespace detail
		{
			void set_data_manager_ptr(data_manager* ptr);
			using exclusive_lock = std::unique_lock<std::shared_mutex>;
			//TODO: return data_manager& here instead; as user is not permitted to store the result
			using data_manager_exclusive = std::tuple<data_manager*, exclusive_lock>;
			data_manager_exclusive get_data_manager_exclusive_lock();
		}

		//===Shared functions, these can be used without blocking other shared threads===
		//returns true if there is a resource associated with this ID
		//this will return false even if a name has been associated with the id
		//this will only test that a resource structure has been created.
		bool exists(unique_id id);
		string get_as_string(unique_id id);
		//returns UniqueId::zero if the name cannot be assiciated with an id
		unique_id get_uid(std::string_view name);

		template<typename T>
		std::vector<unique_id> get_uid(const std::vector<const T*>&);

		//===Exclusive Functions: this will block all threads===
		//NOTE: Can throw hades::data::resource_null
		//		or hades::data::resource_wrong_type
		//	always returns a valid ptr
		template<class T>
		const T* get(unique_id id);

		template<class T>
		data_manager::try_get_return<const T> try_get(unique_id id);

		//refresh requests

		//returns the uid associated with this name
		//or associates the name with a new id if it isn't already
		unique_id make_uid(std::string_view name);		
	}

	string to_string(unique_id value);
	template<>
	unique_id from_string<unique_id>(std::string_view value);
}//hades

#include "hades/detail/data.inl"

#endif //!HADES_DATA_HPP
