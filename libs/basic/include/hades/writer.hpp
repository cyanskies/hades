#ifndef HADES_WRITER_HPP
#define HADES_WRITER_HPP

#include <memory>
#include <string_view>

#include "hades/exceptions.hpp"
#include "hades/string.hpp"

// writer for producing configuration files.

// NOTE: start_sequence("sequence") produces output like this
//	sequence:
//		- item1
//		- item2
//		- item3

// you can generate content differently using the following pattern
/*	write("sqeuence"); start_sequence();
*	write("items")...
*	end_sequence();
* 
*	squence: [item1, item2, item3]
*/

namespace hades::data
{
	//TODO: inherit from hades error
	class write_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	//data writer interface
	class writer
	{
	public:
		virtual ~writer() noexcept = default;

		virtual void start_sequence() = 0;
		virtual void start_sequence(std::string_view) = 0;
		virtual void end_sequence() = 0;
		virtual void start_map() = 0;
		virtual void start_map(std::string_view) = 0;
		virtual void end_map() = 0;
		virtual void write(std::string_view) = 0;
		virtual void write(std::string_view key, std::string_view value) = 0;

		virtual string get_string() const = 0;
		//output to stream
		virtual void print(std::ostream&) const = 0;

		// current and prev must both be sorted
		template<typename T, string_transform<T> ToString = nullptr_t>
		void mergable_sequence(std::string_view, const std::vector<T>& current, const std::vector<T>& prev, ToString&&);

		template<typename T, typename = std::enable_if_t<!is_string_v<T>>>
			requires requires (T ty) {
			to_string(ty);
		}
		void start_map(T value)
		{
			start_map(to_string(value));
		}

		template<typename T, typename U, typename = std::enable_if_t<!is_string_v<T> || !is_string_v<U>>>
			requires requires (T ty, U u) {
			to_string(ty);
			to_string(u);
		}
		void write(T key, U value)
		{
			write(to_string(key), to_string(value));
		}

		template<typename T, typename = std::enable_if_t<!is_string_v<T>>>
			requires requires (T ty) {
			to_string(ty);
		}
		void write(T value)
		{
			write(to_string(value));
		}
	};

	inline std::ostream& operator<<(std::ostream& os, const writer& w)
	{
		w.print(os);
		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, const std::unique_ptr<writer>& w) 
	{
		return os << *w;
	}

	using make_writer_f = std::unique_ptr<writer>(*)();
	
	void set_default_writer(make_writer_f);
	std::unique_ptr<writer> make_writer();
}

#endif // !HADES_WRITER_HPP
