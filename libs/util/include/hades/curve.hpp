#ifndef HADES_UTIL_CURVE_HPP
#define HADES_UTIL_CURVE_HPP

#include <algorithm>
#include <cassert>
#include <deque>
#include <map>
#include <stdexcept>
#include <tuple>

#include "hades/time.hpp"
#include "hades/types.hpp"
#include "hades/utility.hpp"

//curves allow values to interpolated by comparing keyframes
// curve types
//	linear: data between keyframes is exactly the difference between them
//	step: data between keyframes is identical to the previous keyframe
//	pulse: keyframes only exist on the extact time slice they were written to
//	const: normal data


namespace hades 
{
	enum class keyframe_style : uint8 {
		linear,
		step,
		pulse,
		const_t
	};

	template<typename T>
	class basic_curve
	{
	public:
		void reserve(std::size_t s)
		{
			_data.reserve(s);
		}

	protected:
		std::vector<T> _data;
	};

	template<typename T>
	class linear_curve
	{
	public:
		using value_type = T;

		void add_keyframe(time_point, T) {}
		T get(time_point) { return T{}; }
	};

	template<typename T>
	class step_curve
	{
	public:
		using value_type = T;

		void add_keyframe(time_point, T) {}
		T get(time_point) { return T{}; }
	};

	template<typename T>
	class pulse_curve
	{
	public:
		using value_type = T;
		void add_keyframe(time_point, T) {}
		T get(time_point) { return T{}; }
	};

	template<typename T>
	class const_curve
	{
	public:
		using value_type = T;

		T& get() noexcept
		{
			return data;
		}

		const T& get() const noexcept
		{
			return data;
		}

		void set(T t) noexcept
		{
			data = std::move(t);
			return;
		}
	private:
		T data;
	};

	template<typename T>
	struct game_property_t
	{
		using value_type = T;

		constexpr game_property_t() noexcept(std::is_nothrow_default_constructible_v<T>) = default;
		constexpr game_property_t(time_point t, T v) noexcept(std::is_nothrow_copy_constructible_v<T>) 
			: first_time{ t }, second_time{ t }, first{ v }, second{ v } {}

		constexpr void set(time_point t, T v) noexcept(std::is_nothrow_move_assignable_v<T>)
		{
			if (t <= second_time)
			{
				if (t <= first_time)
				{
					first = std::move(v);
					first_time = t;
					return;
				}

				second = std::move(v);
				second_time = t;
				return;
			}

			std::swap(first, second);
			std::swap(first_time, second_time);
			second_time = t;
			second = std::move(v);
			return;
		}

		template<typename U = T, std::enable_if_t<lerpable_v<U> && std::is_same_v<U, T>, int> = 0>
		constexpr T get(time_point t) const noexcept(std::is_nothrow_copy_constructible_v<T>)
		{
			// for lerpable types(usually floats or vectors of floats), values with be lerpped to the time point
			// this is the linear curve

			if (first_time == second_time)
				return first;

			assert(first_time < second_time);

			if (t > second_time)
				return second;
			else if (t < first_time)
				return first;

			const auto total_duration =
				time_cast<seconds_float>(second_time - first_time);
			const auto dur = time_cast<seconds_float>(t - first_time);
			return lerp(first, second, dur.count() / total_duration.count());
		}

		template<typename U = T, std::enable_if_t<!lerpable_v<U> && std::is_same_v<U, T>, int> = 0>
		constexpr const T& get(time_point t) const noexcept
		{
			// for non-lerpable types, values with use the step type
			if (first_time == second_time)
				return first;

			assert(first_time < second_time);

			if (t < second_time)
				return first;

			return second;
		}

		template<typename U = T, std::enable_if_t<!lerpable_v<U> && std::is_same_v<U, T>, int> = 0>
		constexpr const T& get_ref(time_point t) const noexcept
		{
			//get ref is only available if get() would resolve as a ref
			return get(t);
		}

		T first, second;
		time_point first_time;
		time_point second_time;
	};

	

	template<class T>
	using game_property_curve = game_property_t<T>;
}

#endif //HADES_UTIL_CURVES_HPP
