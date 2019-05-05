#ifndef HADES_ANY_MAP_HPP
#define HADES_ANY_MAP_HPP

#include <any>
#include <exception>
#include <memory>
#include <unordered_map>
#include <variant>

#include "Hades/Types.hpp"

namespace hades 
{
	namespace type_erasure
	{
		class key_null : public std::runtime_error
		{
		public:
			using std::runtime_error::runtime_error;
		};

		//the requested resource isn't of the type it is claimed to be
		class value_wrong_type : public std::runtime_error
		{
		public:
			using std::runtime_error::runtime_error;
		};
	}

	//a map whose value can be any type
	template<class Key>
	class any_map
	{
	public:
		using key_type = Key;
		
		template<class T>
		void set(Key key, T value);

		template<class T>
		T get(Key key) const;

		void erase(Key key);
		bool contains(Key key) const;

		template<class T>
		bool is(Key key) const;

	private:
		using data_map = std::map<key_type, std::any>;
		data_map _bag;
	};

	//like any_map, however acceptable types are constrained to ...Types
	template<typename Key, typename ...Types>
	class var_map
	{
	public:
		using key_type = Key; 
		using value_type = std::variant<Types...>;
		using data_map = std::map<key_type, value_type>;

		template<class T>
		void set(Key key, T value);

		template<class T>
		T get(Key key) const;

		bool contains(Key key) const;

	private:
		data_map _bag;
	};
}

#include "hades/detail/any_map.inl"

#endif // !HADES_ANY_MAP_HPP
