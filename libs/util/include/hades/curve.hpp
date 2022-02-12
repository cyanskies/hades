#ifndef HADES_UTIL_CURVE_HPP
#define HADES_UTIL_CURVE_HPP

#include <algorithm>
#include <cassert>
#include <vector>

#include "hades/time.hpp"
#include "hades/utility.hpp"

//curves allow values to interpolated by comparing keyframes
// curve types
//	linear: data between keyframes is exactly the difference between them
//	step: data between keyframes is identical to the previous keyframe
//	pulse: keyframes only exist on the extact time slice they were written to
//	const: normal data

namespace hades::detail
{
	template<typename Vector>
	auto curve_get_near_impl(Vector& v,  time_point t) noexcept
	{
		const auto beg = std::begin(v);
		const auto end = std::end(v);
		assert(beg != end);

		//return time equal or greater than t
		const auto iter = std::lower_bound(beg, end, t);
		if (iter == beg)
			return std::make_pair(iter, end);

		//if iter == end, this is still correct
		//since we know iter != begin
		const auto pre = std::prev(iter);
		return std::make_pair(pre, iter);
	}
}

namespace hades
{
	struct curve_error : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

	//thrown when attempting to retrieve data
	//from an empty curve
	struct curve_no_keyframes : curve_error
	{
		using curve_error::curve_error;
	};

	enum class keyframe_style : uint8
	{
		linear,
		step,
		pulse,
		const_t,
		end
	};

	// basic_curve
	// implements shared behaviour for some of the curve types
	template<typename T>
	class basic_curve
	{
	public:
		void reserve(const std::size_t s)
		{
			_data.reserve(s);
		}

		constexpr bool empty() const noexcept
		{
			return std::empty(_data);
		}

		T& add_keyframe(time_point t, T val)
		{
			if (empty()) // curves should always at least have a starting value
			{
				return _data.emplace_back( keyframe{ t, std::move(val) }).value;
			}

			const auto near = _get_near(t);
			if (near.second != _end() && near.second->time == t)
			{
				near.second->value = std::move(val);
				return near.second->value;
			}
			else
				return _data.insert(near.second, { t, std::move(val) })->value;
		}

		//removes all keyframes after time_point
		void replace_keyframes(time_point t, T val)
		{
			if (empty())
			{
				_data.push_back({ t, std::move(val) });
				return;
			}

			const auto end = std::end(_data);
			const auto iter = std::lower_bound(begin(_data), end);
			if (iter != end)
				_data.erase(iter, end);
			_data.push_back({ t, std::move(val) });
			return;
		}

	protected:
		struct keyframe
		{
			time_point time;
			T value;

			constexpr bool operator<(const time_point& other) const noexcept
			{
				return time < other;
			}
		};

		using data_t = std::vector<keyframe>;
		using get_near_return = std::pair<typename data_t::iterator, typename data_t::iterator>;
		get_near_return _get_near(time_point t) noexcept
		{
			return detail::curve_get_near_impl(_data, t);
		}

		using get_near_return_const = std::pair<typename data_t::const_iterator, typename data_t::const_iterator>;
		get_near_return_const _get_near(time_point t) const noexcept
		{
			return detail::curve_get_near_impl(_data, t);
		}

		typename data_t::const_iterator _end() const noexcept
		{
			return end(_data);
		}

		std::vector<keyframe> _data;
	};

	template<typename T>
	class linear_curve : public basic_curve<T>
	{
	public:
		using value_type = T;

		// throws curve_no_keyframes
		T get(time_point t) const noexcept
		{
			using basic = basic_curve<T>;
			const auto frames = basic::_get_near(t);
			if (frames.second == basic::_end())
				return frames.first->value;

			const auto start_time = frames.first->time;
			assert(t >= start_time);
			const auto end_time = frames.second->time - start_time;
			const auto current_time = t - start_time;

			const auto lerp_progress =
				static_cast<float>(current_time.count()) /
				static_cast<float>(end_time.count());

			return lerp(frames.first->value, frames.second->value, lerp_progress);
		}
	};

	template<typename T>
	class step_curve : public basic_curve<T>
	{
	public:
		using value_type = T;

		const T& get(time_point t) const noexcept
		{
			using basic = basic_curve<T>;
			assert(!basic_curve<T>::empty());
			const auto n = basic::_get_near(t);
			if (n.second == basic::_end())
				return n.first->value;
			return n.second->value;
		}
	};

	template<typename T>
	class pulse_curve : public basic_curve<T>
	{
	public:
		using value_type = T;
		struct pulse_keyframe
		{
			T value;
			hades::time_point time;

			bool operator!=(const pulse_keyframe& rhs) const noexcept
			{
				return std::tie(time, value) != std::tie(rhs.time, rhs.value);
			}

			bool operator==(const pulse_keyframe& rhs) const noexcept
			{
				return std::tie(time, value) == std::tie(rhs.time, rhs.value);
			}
		};

		//NOTE: this cannot be constexpr untill vector supports constexpr
		static inline const auto bad_keyframe = 
			pulse_keyframe{ {}, hades::time_point::max() };

		// first is equal to or before t,
		// second is after t or bad_keyframe if
		// no later frames exist
		std::pair<pulse_keyframe, pulse_keyframe> get(time_point t) const noexcept
		{
			auto iters = basic_curve<T>::_get_near(t);
			return { { iters.first->value, iters.first->time },
				(iters.second == basic_curve<T>::_end()) ? bad_keyframe :
				pulse_keyframe{ iters.second->value, iters.second->time }
			};
		}

		// TODO: implement
		std::vector<pulse_keyframe> get_range(time_point min, time_point max) const
		{
			if (min > max) return get_range(max, min);

			auto out = std::vector<pulse_keyframe>{};
			//equal_range
			//return out;
		}
	};

	template<typename T>
	using pulse_keyframe = typename pulse_curve<T>::pulse_keyframe;

	//TODO: tempted to rename this to static_curve
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
	struct is_linear_curve : std::false_type {};
	template<typename T>
	struct is_linear_curve<linear_curve<T>> : std::true_type {};
	template<typename T>
	constexpr auto is_linear_curve_v = is_linear_curve<T>::value;

	template<typename T>
	struct is_step_curve : std::false_type {};
	template<typename T>
	struct is_step_curve<step_curve<T>> : std::true_type {};
	template<typename T>
	constexpr auto is_step_curve_v = is_step_curve<T>::value;

	template<typename T>
	struct is_pulse_curve : std::false_type {};
	template<typename T>
	struct is_pulse_curve<pulse_curve<T>> : std::true_type {};
	template<typename T>
	constexpr auto is_pulse_curve_v = is_pulse_curve<T>::value;

	template<typename T>
	struct is_const_curve : std::false_type {};
	template<typename T>
	struct is_const_curve<const_curve<T>> : std::true_type {};
	template<typename T>
	constexpr auto is_const_curve_v = is_const_curve<T>::value;
}


#endif //!HADES_UTIL_CURVES_HPP
