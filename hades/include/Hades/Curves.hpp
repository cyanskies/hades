#ifndef HADES_CURVES_HPP
#define HADES_CURVES_HPP

#include <algorithm>
#include <cassert>
#include <set>

//A set of curve classes for variables
//curves allow values to interpolated by comparing keyframes
//keyframes should be estimated in advance so that clients can use them for prediction
//keyframes can be overridden as the simulation catches up to them for accuracy

namespace hades {
	template<typename Time, typename Data>
	struct Keyframe
	{
		Keyframe(Time at) : t(at) {}

		Time t;
		Data value;
	};

	template<typename Time, typename Data>
	bool operator<(const Keyframe<Time, Data> &lhs, const Keyframe<Time, Data> &rhs)
	{
		return lhs.t < rhs.t;
	}

	enum CurveType {
		CONST, //data is constant for any keyframe
		LINEAR, //data between keyframes is exactly the difference between them
		STEP, // data between keyframes is identical to the previous keyframe
		PULSE // data between keyframes is null
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
			if (_type == CurveType::CONST)
			{
				//replace the only value with the new one
				data.insert({ Time(), value });
			}
			else
			{
				auto at = data.insert({ at, value });

				//if the insertion was successful and isn't the last element
				//in the container
				if (at.second && at.first != data.rbegin())
					//erase everything after the newly inserted keyframe
					data.erase(++at.first, data.end());
			}
		}

		Data get(Time at)
		{
			if (_type == CurveType::CONST)
			{
				//TODO: handle unasigned curve
				//will probably have to throw, or return Data();
				return (*data.begin()).value;
			}
			else if (_type == CurveType::LINEAR)
			{
				//TODO: handle returning from a time before the first frame, or after the last
				//TODO: provide a overridable lerp function for this to fall back on?

				//linear imp goes here
				//auto d = GetRange(at);

				//return lerp of d.first to d.second with at.
				assert(false && "unimplemented");

				return Data();
			}
			else if (_type == CurveType::PULSE)
			{
				assert(_type != CurveType::PULSE);

				//throw, can use at for pulse
			}
			else if (_type == CurveType::STEP)
			{
				//todo: if at is past the end, return second
				auto d = GetRange(at);

				return (*d.first).value;
			}
			else
			{
				throw curve_error("Malformed curve, curvetype was: " + _type);
			}
		}

		//These are the only valid ways to get data from a Pulse
		using DataInfo = std::pair<Time, Data>;
		//returns the closest frame before at
		DataInfo getPrevious(Time at)
		{
			//TODO: return <bool, Datainfo> tuple
			//as thier might not be a previous
			auto d = GetRange(at);

			return d.first;
		}
		//returns the closest frame after at
		DataInfo getNext(Time at)
		{
			//same as above, need to handle their being no frame after at
			auto d = GetRange(at);

			return d.second;
		}

		//returns all keyframes between the specified times
		std::vector<DataInfo> getBetween(Time first, Time second)
		{
			return std::vector<DataInfo>();
		}

		//For converting to the usable Curve Types
		CurveType type() { return _type; }

	private:
		using FrameType = Keyframe<Time, Data>;
		using DataType = std::set< FrameType >;
		using IterPair = std::pair<typename DataType::iterator, typename DataType::iterator>;

		//returns the keyframes either side of 'at'
		IterPair GetRange(Time at) const
		{
			//auto next = std::lower_bound(data.begin(), data.end(), at);
			//return IterPair(--next, next);
			return IterPair();
		}

		DataType data;

	private:
		CurveType _type;
	};
}

#endif //HADES_CURVES_HPP
