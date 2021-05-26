#ifndef HADES_WRITER_HPP
#define HADES_WRITER_HPP

#include <string_view>

#include "hades/exceptions.hpp"
#include "hades/string.hpp"

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

		template<typename T, typename = std::enable_if_t<!is_string_v<T>>>
		void start_map(T value)
		{
			start_map(to_string(value));
		}

		template<typename T, typename U, typename = std::enable_if_t<!is_string_v<T> || !is_string_v<U>>>
		void write(T key, U value)
		{
			write(to_string(key), to_string(value));
		}

		template<typename T, typename = std::enable_if_t<!is_string_v<T>>>
		void write(T value)
		{
			write(to_string(value));
		}
	};

	using make_writer_f = std::unique_ptr<writer>(*)();
	
	void set_default_writer(make_writer_f);
	std::unique_ptr<writer> make_writer();
}

#endif // !HADES_WRITER_HPP
