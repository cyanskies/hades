#ifndef HADES_UTIL_CURVE_HPP
#define HADES_UTIL_CURVE_HPP

#include <algorithm>
#include <cassert>
#include <map>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "hades/time.hpp"
#include "hades/types.hpp"

//A set of curve classes for variables
//curves allow values to interpolated by comparing keyframes
//keyframes should be estimated in advance so that clients can use them for prediction
//keyframes can be overridden as the simulation catches up to them for accuracy

namespace hades {
	//TODO: move lerp into the utility header
	//define lerpable
	template<typename T, typename std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
	T lerp(T first, T second, float alpha) noexcept
	{
		return (1.f - alpha) * first + alpha * second;
	}

	template<typename T, typename std::enable_if_t<!std::is_arithmetic_v<T>, int> = 0>
	T lerp(T first, T second, float alpha)
	{
		throw std::logic_error{"called lerp with a non-arithmetic type"};
	}

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

	class curve_error : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	template<typename Data>
	class basic_curve final
	{
	public:
		using Time = time_point;
		using frame_t = std::pair<Time, Data>;
		using DataType = std::vector<frame_t>;

		basic_curve() = default;
		explicit basic_curve(curve_type type) : _type{type}
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
			assert(_type != curve_type::pulse);

			if (_type == curve_type::pulse)
				throw curve_error("Don't use curve::get() for a pulse curve. Use get* functions instead.");

			if (_data.empty())
				throw curve_error("Tried to call get() on an empty curve");

			//if we're before the start of the data or after the end
			// then just return the closest keyframe
			const auto last = --_data.end();

			if (at < _data.begin()->first)
				return _data.begin()->second;
			else if (last->first < at)
				return last->second;

			if (_type == curve_type::const_c)
			{
				if (_data.empty())
					throw curve_error("Tried to read from empty CONST curve");

				return _data.begin()->second;
			}
			else if (_type == curve_type::linear)
			{
				const auto [a, b] = _getRange(at);

				const auto first = at - a->first;
				const auto second = b->first - a->first;

				const float interp = static_cast<float>(first.count()) / static_cast<float>(second.count());

				using hades::lerp;
				return lerp(a->second, b->second, interp);
			}
			else if (_type == curve_type::step)
			{
				const auto [a,b] = _getRange(at);

				if (b->first <= at)
					return b->second;

				return a->second;
			}

			throw curve_error("Malformed curve");
		}

		//These are the only valid ways to get data from a Pulse
		//returns the closest frame before at
		frame_t getPrevious(Time at) const
		{
			auto d = _getRange(at);
			return *d.first;
		}

		//returns the closest frame after at
		frame_t getNext(Time at) const
		{
			auto d = _getRange(at);
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
			if (_type == curve_type::const_c)
			{
				if (std::empty(_data))
					_data.emplace_back(Time{}, std::move(value));
				else
					_data[0] = frame_t{ Time{}, std::move(value) };
			}
			else
			{
				const auto location = _getRange(at);
				const auto iter = _data.emplace(location.second, frame_t{ at, std::move(value) });
				//if the insertion was successful and isn't the last element
				//in the container
				if (const auto target = std::next(iter);
					erase && target != std::end(_data))
					//erase everything after the newly inserted keyframe
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
				return IterPair{ std::begin(_data), std::begin(_data) };

			//what can lower bound throw?(bad_alloc) this prevents the method being noexcept
			auto next = std::lower_bound(_data.begin(), _data.end(), keyframe<Data>{ at, Data{} }, keyframe_less<Data>);
			if (next == _data.end())
				next = ++_data.begin();

			return IterPair{ std::prev(next), next };
		}

		DataType _data;
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
