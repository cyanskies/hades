#ifndef HADES_DATA_HPP
#define HADES_DATA_HPP

#include <functional>
#include <map>
#include <optional>
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
		// parser func accepts the current mod as unique_id
		using parser_func = void(*)(unique_id, const data::parser_node&, data::data_manager&);
		
		class resource_link_base
		{
		public:
			explicit resource_link_base(unique_id id) noexcept : _id{ id } {}

			virtual ~resource_link_base() noexcept = default;

			virtual void update_link(data::data_manager&) = 0;

			unique_id id() const noexcept
			{
				return _id;
			};

			void add_reverse_link(const unique_id i)
			{
				if (std::none_of(begin(_reverse_links), end(_reverse_links), [i](auto&& other) {
					return other == i;
					}))
					_reverse_links.emplace_back(i);
				return;
			}

			const std::vector<unique_id> get_reverse_links() const noexcept
			{
				return _reverse_links;
			}

			virtual operator bool() const noexcept = 0;

		private:
			unique_id _id = unique_zero;
			std::vector<unique_id> _reverse_links;
		};

		// resources use these to point to one another
		// this lets the ptrs be updated from the data manager
		template<typename T>
		class resource_link_type : public resource_link_base
		{
		public:
			// get_func should be a function that gets the pointer to the resource without loading it
			// eg. call get<> with the no_load tag
			using get_func = const T* (*)(data::data_manager&, unique_id);

			constexpr explicit resource_link_type(unique_id id, get_func get) noexcept :
				resource_link_base{ id }, _get{ get } {}

			void update_link(data::data_manager&) override;

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

			bool is_linked() const noexcept
			{
				return _link;
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

			template<typename U>
			friend bool operator<(const resource_link<U>& lhs, const resource_link<U>& rhs) noexcept;
			template<typename U>
			friend bool operator==(const resource_link<U>& lhs, const resource_link<U>& rhs) noexcept;

		private:
			const resource_link_type<T>* _link = nullptr;
		};

		template<typename T>
		bool operator<(const resource_link<T>& lhs, const resource_link<T>& rhs) noexcept
		{
			return lhs._link < rhs._link;
		}

		template<typename T>
		bool operator==(const resource_link<T>& lhs, const resource_link<T>& rhs) noexcept
		{
			return lhs._link == rhs._link;
		}

		template<typename T>
		std::vector<unique_id> get_ids(const std::vector<resource_link<T>>&);
	}

	namespace data
	{
		//tag type to request that a resource not be lazy loaded
		struct no_load_t {};
		constexpr no_load_t no_load{};

		struct mod
		{
			unique_id id;
			std::vector<unique_id> dependencies;
			std::vector<string> includes;

			string name;
			// TODO: filesystem::path
			string source; // path to source
		};

		namespace detail
		{
			template<class T>
			const T* get_no_load(data::data_manager&, unique_id);
		}

		class data_manager
		{
		public:
			data_manager();

			virtual ~data_manager() = default;

			virtual void register_resource_type(std::string_view name, resources::parser_func parser) = 0;

			//returns a resource if it exists, or creates it if it doesn't
			//this should be used when writing parsers
			//allows creating a ptr to a resource that hasn't actually been defined yet
			// returns nullptr on error
			template<class T>
			[[nodiscard]] T* find_or_create(unique_id target, std::optional<unique_id> mod = {}, std::string_view group = {});

			template<typename T>
			[[nodiscard]] std::vector<T*> find_or_create(const std::vector<unique_id>& target, std::optional<unique_id> mod = {}, std::string_view group = {});

			//returns true if the id has a resource associated with it
			bool exists(unique_id id) const;

			template<typename T>
			resources::resource_link<T> make_resource_link(unique_id, unique_id from, typename resources::resource_link_type<T>::get_func = detail::get_no_load<T>);
			template<typename T>
			std::vector<resources::resource_link<T>> make_resource_link(const std::vector<unique_id>&, unique_id from, typename resources::resource_link_type<T>::get_func = detail::get_no_load<T>);

			//returns the resource associated with the id
			//returns the resource base class
			//throws resource_null if the id doesn't refer to a resource
			resources::resource_base* get_resource(unique_id id, std::optional<unique_id> mod = {});
			//as above, returns null if the id doesn't refer to a resource
			resources::resource_base* try_get_resource(unique_id id, std::optional<unique_id> mod = {}) noexcept;
			//gets a non-owning ptr to the resource represented by id
			//if the reasource has not been loaded it will be loaded before returning
			// throws resource_null or resource_wrong_type
			template<class T>
			T* get(unique_id id, std::optional<unique_id> = {});

			//gets a non-owning ptr to the resource represented by id
			// throws resource_null or resource_wrong_type
			template<class T>
			T* get(unique_id id, const no_load_t, std::optional<unique_id> = {});

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
			try_get_return<T> try_get(unique_id id, std::optional<unique_id> = {}) noexcept;

			//gets a non-owning ptr to the resource represented by id
			template<class T>
			try_get_return<T> try_get(unique_id id, const no_load_t, std::optional<unique_id> = {}) noexcept;

			void update_all_links();

			void erase(unique_id id, std::optional<unique_id> mod = {}) noexcept;

			using resource_group = std::pair<string, std::vector<const resources::resource_base*>>;
			struct resource_storage
			{
				mod mod_info;
				std::vector<resource_group> resources_by_type;
			};

			virtual bool try_load_mod(std::string_view) = 0;

			void push_mod(mod);
			void pop_mod();

			const mod& get_mod(unique_id) const;
			mod& get_mod(unique_id);

			static std::string_view built_in_mod_name() noexcept;
			[[nodiscard]] bool is_built_in_mod(unique_id) const noexcept;

			[[nodiscard]] std::size_t get_mod_count() const noexcept;
			[[nodiscard]] std::vector<resource_storage*> get_mod_stack();

			std::vector<std::string_view> get_all_names_for_type(std::string_view resource_type, std::optional<unique_id> mod = {}) const;

			//refresh functions request that the mentioned resources be pre-loaded
			virtual void refresh() = 0;
			virtual void refresh(unique_id) = 0;

			virtual const string& get_as_string(unique_id id) const noexcept = 0;
			virtual unique_id get_uid(std::string_view name) const = 0;
			virtual unique_id get_uid(std::string_view name) = 0;

		private:
			struct mod_storage : resource_storage
			{
				std::vector<std::unique_ptr<resources::resource_base>> resources;
			};

			mod_storage& _get_mod(unique_id);
			//creates a resource with the value of ptr
			//and assigns it to the name id
			//throws resource_name_already_used if the id already refers to a resource
			template<class T>
			void _set(unique_id id, std::unique_ptr<resources::resource_base> ptr, std::string_view group);

			std::vector<mod_storage> _mod_stack;
			std::vector<std::unique_ptr<resources::resource_link_base>> _resource_links;
		};

		namespace detail
		{
			void set_data_manager_ptr(data_manager* ptr);
			using exclusive_lock = std::unique_lock<std::shared_mutex>;
			//TODO: return data_manager& here instead; as user is not permitted to store the result
			using data_manager_exclusive = std::tuple<data_manager*, exclusive_lock>;
			data_manager_exclusive get_data_manager_exclusive_lock();
		}

		// TODO: move these to the top of the file
		//===Shared functions, these can be used without blocking other shared threads===
		//returns true if there is a resource associated with this ID
		bool exists(unique_id id);
		const string& get_as_string(unique_id id);
		//returns UniqueId::zero if the name cannot be assiciated with an id
		unique_id get_uid(std::string_view name);
		const mod& get_mod(unique_id id);

		//===Exclusive Functions: this will block all threads===
		//NOTE: Can throw hades::data::resource_null
		//		or hades::data::resource_wrong_type
		//	always returns a valid ptr
		template<class T>
		const T* get(unique_id id, std::optional<unique_id> mod = {});
		template<class T>
		const T* get(unique_id id, const no_load_t, std::optional<unique_id> mod = {});

		template<class T>
		data_manager::try_get_return<const T> try_get(unique_id id, std::optional<unique_id> mod = {});

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
