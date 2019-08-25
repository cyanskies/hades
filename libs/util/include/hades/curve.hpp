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

//A set of curve classes for variables
//curves allow values to interpolated by comparing keyframes
//keyframes should be estimated in advance so that clients can use them for prediction
//keyframes can be overridden as the simulation catches up to them for accuracy

//TODO: rewrite this whole class so the logic makes more sense

namespace hades {
	template<typename T>
	using keyframe = std::pair<time_point, T>;
	
	template<typename T>
	constexpr bool keyframe_less(const keyframe<T> &a, const keyframe<T> &b) noexcept
	{
		return a.first < b.first;
	}

	enum class curve_type {
		error, //something is wrong
		const_c, //data is constant for any keyframe
		linear, //data between keyframes is exactly the difference between them
		step, // data between keyframes is identical to the previous keyframe
		pulse // data between keyframes is null
	};

	//TODO: base on hades error
	class curve_error : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	template<typename Data>
	class basic_curve final
	{
	public:
		using value_type = Data;
		using Time = time_point;
		using frame_t = std::pair<Time, Data>;
		using DataType = std::deque<frame_t>;

		basic_curve() = default;
		explicit basic_curve(curve_type type) : _type{type}
		{}

		basic_curve(curve_type type, Data value) : _type{type}, 
			_data{ {Time{}, std::move(value)} }
		{}

		static constexpr auto is_curve_nothrow_move_assignable_v = std::conjunction<
            std::is_nothrow_swappable<DataType>,
			std::is_nothrow_copy_assignable<curve_type>>::value;

		basic_curve(const basic_curve &other) :
			_type{other._type}, _data{other._data}
		{}

		basic_curve(basic_curve &&other)
			noexcept(is_curve_nothrow_move_assignable_v) :
			_type{other._type}, _data{std::move(other._data)}
		{}

		basic_curve &operator=(const basic_curve &other)
		{
			basic_curve<Data> c{ other };
			using std::swap;
			swap(c._data, _data);
			_type = c._type;
			return *this;
		}

		basic_curve &operator=(basic_curve &&other)
			noexcept(is_curve_nothrow_move_assignable_v)
		{
			using std::swap;
			swap(_data, other._data);
			swap(_type, other._type);
			return *this;
		}

		//adds a keyframe
		//when you add a keyframe, all keyframes after it are erased
		void set(Time at, Data value)
		{
			_insertFrame(at, std::move_if_noexcept(value), true);
		}

		//inserting a keyframe doesn't remove other frames, but may still replace a frame at the same time as the new frame.
		void insert(Time at, Data value)
		{
			_insertFrame(at, value);
		}

		Data get(Time at) const
		{
			return get_ref(at);
		}

		//FIXME: find solution for this,
		// need to be able to return lerped data
		// without breaking reference return?
		const Data& get_ref(Time at) const
		{
			assert(_type != curve_type::pulse);

			if (_type == curve_type::pulse)
				throw curve_error("Don't use curve::get() for a pulse curve. Use get* functions instead.");

			if (_data.empty())
				throw curve_error("Tried to call get() on an empty curve");

			if (std::size(_data) == 1)
				return _data[0].second;
			//if we're before the start of the data or after the end
			// then just return the closest keyframe
			const auto last = --_data.end();

			if (at < _data[0].first)
				return _data[0].second;
			else if (last->first < at)
				return last->second;

			if (_type == curve_type::const_c)
			{
				//TODO: catch bad access rather than test extra here?
				if (_data.empty())
					throw curve_error("Tried to read from empty CONST curve");

				return _data.front().second;
			}
			else if (_type == curve_type::linear)
			{
				const auto [a, b] = _getRange(at);

				if (b == std::end(_data))
					return a->second;

				const auto first = at - a->first;
				const auto second = b->first - a->first;

				const float interp = static_cast<float>(first.count()) / static_cast<float>(second.count());

				using hades::lerp;
				_const_storage = lerp(a->second, b->second, interp);
				return _const_storage;
			}
			else if (_type == curve_type::step)
			{
				const auto [a,b] = _getRange(at);

				if (b != std::end(_data) && b->first <= at)
					return b->second;

				return a->second;
			}
			else
				throw curve_error("Malformed curve");
		}

		//These are the only valid ways to get data from a Pulse
		//returns the closest frame before at
		frame_t getPrevious(Time at) const
		{
			//TODO: FIXME: if the curve has no keyframes then d.first might be std::end
			//std::optional?
			auto d = _getRange(at);
			return *d.first;
		}

		//returns the closest frame after at
		frame_t getNext(Time at) const
		{
			auto d = _getRange(at);
			//FIXME: TODO: d.second might be std::end	
			return *d.second;
		}

		//returns all keyframes between the specified times
		std::vector<frame_t> getBetween(Time first, Time second) const
		{
			auto begin = this->begin(), end = this->end();
			auto lower = std::lower_bound(begin, end, keyframe<Data>{ first, Data{} }, keyframe_less<Data>);
			if (lower == end)
				lower = begin;
			auto upper = std::upper_bound(begin, end, keyframe<Data>{ second, Data{} }, keyframe_less<Data>);
			if (upper == end &&
				lower != upper)
				--upper;

			std::vector<keyframe<Data>> output;
			output.reserve(std::distance(lower, upper));

			while (lower != upper)
				output.push_back(*lower++);

			return output;
		}

		using const_iterator = typename DataType::const_iterator;

		[[nodiscard]] bool empty() const noexcept
		{
			return _data.empty();
		}

		const_iterator begin() const noexcept
		{
			return _data.begin();
		}

		const_iterator end() const noexcept
		{
			return _data.end();
		}

		//For converting to the usable Curve Types
		curve_type type() const noexcept { return _type; }

		template<typename D>
		friend bool operator==(const basic_curve<D> &lhs, const basic_curve<D> &rhs) noexcept;
		template<typename D>
		friend bool operator!=(const basic_curve<D> &lhs, const basic_curve<D> &rhs) noexcept;

	private:
		using IterPair = std::pair<typename DataType::const_iterator, typename DataType::const_iterator>;

		void _insertFrame(Time at, Data value, bool erase = false)
		{
			//TODO: optimise for time > end and value == end.value
			auto iter = std::end(_data);

			if (_type == curve_type::const_c)
			{
				if (std::empty(_data))
					_data.emplace_back(at, std::move(value));
				else
					_data[0] = frame_t{ at, std::move(value) };
				return;
			}

			const auto [pre, post] = _getRange(at);
				
			if (_type == curve_type::step)
			{
				if (pre != std::end(_data)
					&& value == pre->second)
					return;
			}
			else if (_type == curve_type::linear)
			{
				if constexpr (lerpable_v<Data>)
				{
					if (pre != std::end(_data) && std::size(_data) > 1)
					{
						if (post != std::end(_data))
						{
							const auto total_duration =
								time_cast<seconds_float>(post->first - pre->first);
							const auto dur = time_cast<seconds_float>(at - pre->first);

							const auto lerp_value = lerp(pre->second,
								post->second, dur.count() / total_duration.count());

							//if adding this keyframe wouldn't change the,
							//value at any time, then we can just skip it
							if (float_near_equal(lerp_value, value))
								return;
						}
						else
						{
							const auto prepre = std::prev(pre);
							const auto total_duration = time_cast<seconds_float>(at - prepre->first);
							const auto dur = time_cast<seconds_float>(pre->first - prepre->first);
							const auto lerp_value = lerp(prepre->second, value, dur.count() / total_duration.count());

							//if this new keyframe just continues a linear progression
							// then we can calculate the old 'last' frame
							// by lerping the one before it and the new last frame
							// so we'll just replace the old last frame with the new one
							if (float_near_equal(lerp_value, pre->second))
							{
								const auto pos = std::distance(std::cbegin(_data), pre);
								//no need to move, lerpable is floating point
								_data[pos] = { at, value };
								return;
							}
						}
					}
				}
			}

			if (pre != std::end(_data) && pre->first == at)
			{
				const auto pos = std::distance(std::cbegin(_data), pre);
				_data[pos].second = std::move(value);
			}
			else
				iter = _data.emplace(post, frame_t{ at, std::move(value) });

			if (iter != std::end(_data))
			{
				const auto target = std::next(iter);
				//erase everything after the newly inserted keyframe
				if(erase && target != std::end(_data))
					_data.erase(target, std::end(_data));
			}

			return;
		}

		//returns the keyframes either side of 'at'
		IterPair _getRange(Time at) const
		{
			if (std::empty(_data))
				return IterPair{ std::end(_data), std::end(_data) };
			else if (_data.size() == 1)
				return IterPair{ std::begin(_data), std::end(_data) };

			//what can lower bound throw?(bad_alloc) this prevents the method being noexcept
			const auto next = std::upper_bound(_data.begin(), _data.end(), keyframe<Data>{ at, Data{} }, keyframe_less<Data>);
			
			if (next == std::begin(_data))
				return { next, next };

			return IterPair{ std::prev(next), next };
		}

		DataType _data;
		// stores the result of lerp in a linear curve
		// so that the result can be returned by ref
		mutable Data _const_storage;
		curve_type _type = curve_type::error;
	};

	template<typename T>
	bool operator==(const basic_curve<T> &lhs, const basic_curve<T> &rhs) noexcept
	{
		return lhs._type == rhs._type && lhs._data == rhs._data;
	}

	template<typename T>
	bool operator!=(const basic_curve<T> &lhs, const basic_curve<T> &rhs) noexcept
	{
		return !(rhs == lhs);
	}

	template<class T>
	using curve = basic_curve<T>;
}

#endif //HADES_UTIL_CURVES_HPP
