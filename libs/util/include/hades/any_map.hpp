#ifndef HADES_ANY_MAP_HPP
#define HADES_ANY_MAP_HPP

#include <any>
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <variant>

#include "hades/types.hpp"

namespace hades 
{
	class any_map_key_null : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	//the requested resource isn't of the type it is claimed to be
	class any_map_value_wrong_type : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	//a map whose value can be any type
	template<class Key>
	class any_map
	{
	public:
		using key_type = Key;
		
		template<class T>
		T& set(Key, T);

		struct allow_overwrite_t {};
		static constexpr allow_overwrite_t allow_overwrite = {};

		template<class T>
		T& set(allow_overwrite_t, Key, T);

		template<class T>
		T get(Key key) const;
		template<class T>
		T& get_ref(Key key);
		template<class T>
		const T& get_ref(Key key) const;
		template<class T>
		T* try_get(Key key) noexcept;
		template<class T>
		const T* try_get(Key key) const noexcept;

		void erase(Key key);
		bool contains(Key key) const noexcept;

		template<class T>
		bool is(Key key) const;

	private:
		using data_map = std::unordered_map<key_type, std::any>;
		data_map _bag;
	};

	//like any_map, however acceptable types are constrained to ...Types
	template<typename Key, typename ...Types>
	class var_map
	{
	public:
		using key_type = Key; 
		using value_type = std::variant<Types...>;
		using data_map = std::unordered_map<key_type, value_type>;

		template<class T>
		T& set(Key key, T value);

		template<class T>
		T get(Key key) const;

		bool contains(Key key) const;

	private:
		data_map _bag;
	};
}

#include "hades/detail/any_map.inl"

#endif // !HADES_ANY_MAP_HPP
