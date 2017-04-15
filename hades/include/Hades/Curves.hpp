#ifndef HADES_CURVES_HPP
#define HADES_CURVES_HPP

#include <algorithm>
#include <cassert>
#include <exception>
#include <set>
#include <tuple>
#include <vector>

//A set of curve classes for variables
//curves allow values to interpolated by comparing keyframes
//keyframes should be estimated in advance so that clients can use them for prediction
//keyframes can be overridden as the simulation catches up to them for accuracy

namespace hades {
	template<typename Time, typename Data>
	struct Keyframe
	{
		Keyframe(Time at) : t(at) {}
		Keyframe(Time at, Data data) : t(at), value(data) {}

		Time t;
		Data value;
	};

	template<typename T>
	T lerp(T first, T second, float aplha)
	{
		return (1 - alpha) * first + t * second;
	}

	template<typename Time, typename Data>
	bool operator==(const Keyframe<Time, Data> &lhs, const Keyframe<Time, Data> &rhs)
	{
		return lhs.t == rhs.t && lhs.value == rhs.value;
	}

	template<typename Time, typename Data>
	bool operator!=(const Keyframe<Time, Data> &lhs, const Keyframe<Time, Data> &rhs)
	{
		return !(lhs == rhs);
	}

	template<typename Time, typename Data>
	bool operator<(const Keyframe<Time, Data> &lhs, const Keyframe<Time, Data> &rhs)
	{
		return lhs.t < rhs.t;
	}

	template<typename Time, typename Data>
	bool operator<(const Keyframe<Time, Data> &lhs, const Time &rhs)
	{
		return lhs.t < rhs;
	}

	template<typename Time, typename Data>
	bool operator<(const Time &lhs, const Keyframe<Time, Data> &rhs)
	{
		return lhs < rhs.t;
	}

	enum CurveType {
		ERROR, //something is wrong
		CONST, //data is constant for any keyframe
		LINEAR, //data between keyframes is exactly the difference between them
		STEP, // data between keyframes is identical to the previous keyframe
		PULSE, // data between keyframes is null
	};

	class curve_error : public std::exception
	{
	public:
		using std::exception::exception;
	};

	template<typename Time, typename Data>
	class Curve final
	{
	public:
		Curve(CurveType type) : _type(type)
		{}

		//adds a keyframe
		//when you add a keyframe, all keyframes after it are erased
		void set(Time at, Data value)
		{
			_insertFrame(at, value, true);
		}

		//inserting a keyframe doesn't remove other frames, but may replace a frame at the same time as the new frame.
		void insert(Time at, Data value)
		{
			_insertFrame(at, value);
		}

		Data get(Time at) const
		{
			assert(_type != CurveType::PULSE);

			if (_type == CurveType::PULSE)
				throw curve_error("Don't use Curve::get() for a pulse curve. Use get* functions instead.");

			//if we're before the start of the data or after the end
			// then just return the closest keyframe
			if (at < *_data.begin())
				return _data.begin()->value;
			else (at > *_data.end())
				return _data.end()->value;
			
			if (_type == CurveType::CONST)
			{
				if (_data.empty())
					throw curve_error("Tried to read from empty CONST curve");

				return _data.begin()->value;
			}
			else if (_type == CurveType::LINEAR)
			{
				auto d = _getRange(at);

				assert(false & "needs more work");
				//interp value = current - first(start of range) / second - first 

				return ::lerp(d.first->value, d.second->value, d.second->t - d.first->t)
			}
			else if (_type == CurveType::STEP)
			{
				//todo: if at is past the end, return second
				auto d = _getRange(at);

				return d.first->value;
			}

			throw curve_error("Malformed curve, curvetype was: " + _type);
		}

		//These are the only valid ways to get data from a Pulse
		using FrameType = Keyframe<Time, Data>;
		//returns the closest frame before at
		FrameType getPrevious(Time at) const
		{
			auto d = _getRange(at);
			return d.first;
		}

		//returns the closest frame after at
		FrameType getNext(Time at) const
		{
			auto d = _getRange(at);
			return d.second;
		}

		//returns all keyframes between the specified times
		std::vector<typename FrameType> getBetween(Time first, Time second) const
		{
			return std::vector<typename FrameType>();
		}

		using DataType = std::set< FrameType >;
		using const_iterator = typename DataType::const_iterator;

		const_iterator begin() const
		{
			return _data.begin();
		}

		const_iterator end() const
		{
			return _data.end();
		}

		//For converting to the usable Curve Types
		CurveType type() { return _type; }

		friend bool operator==(const Curve<Time, Data> &lhs, const Curve<Time, Data> &rhs);
		friend bool operator!=(const Curve<Time, Data> &lhs, const Curve<Time, Data> &rhs);

	private:
		using IterPair = std::pair<typename DataType::iterator, typename DataType::iterator>;

		void _insertFrame(Time at, Data value, bool erase = false)
		{
			if (_type == CurveType::CONST)
			{
				//replace the only value with the new one
				_data.insert({ Time(), value });
			}
			else
			{
				auto iter = _data.insert({ at, value });

				//if the insertion was successful and isn't the last element
				//in the container
				if (erase && iter.second && (iter.first != --_data.end()))
					//erase everything after the newly inserted keyframe
					_data.erase(++iter.first, _data.end());
			}
		}

		//returns the keyframes either side of 'at'
		IterPair _getRange(Time at) const
		{
			if (_data.size() == 1)
				return IterPair(_data.begin(), _data.begin());

			auto next = std::lower_bound(_data.begin(), _data.end(), at);
			if (next == _data.end())
				next = _data.begin() + 1;

			return IterPair(--next, next);
		}

		DataType _data;
		CurveType _type;
	};

	template<typename Time, typename Data>
	bool operator==(const Curve<Time, Data> &lhs, const Curve<Time, Data> &rhs)
	{
		return lhs._type == rhs._type && lhs._data == rhs._data;
	}

	template<typename Time, typename Data>
	bool operator!=(const Curve<Time, Data> &lhs, const Curve<Time, Data> &rhs)
	{
		return !(rhs == lhs);
	}

	template<typename Time, typename Data>
	std::vector<Keyframe<Time, Data>> GetRange(const Curve<Time, Data> &curve, Time first, Time second)
	{
		auto begin = curve.begin(), end = curve.end();
		auto lower = std::lower_bound(begin, end, first);
		if (lower == end)
			lower = begin;
		auto upper = std::upper_bound(begin, end, second);
		if (upper == end)
			--upper;

		std::vector<Keyframe<Time,Data>> output;

		while(lower != upper)
			output.push_back(*lower);

		return output;
	}
}

#endif //HADES_CURVES_HPP
