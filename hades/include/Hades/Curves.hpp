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
	//TODO: comparison function for this type
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

	template<typename Time, typename Data>
	class ConstCurve;
	template<typename Time, typename Data>
	class LinearCurve;
	template<typename Time, typename Data>
	class StepCurve;
	template<typename Time, typename Data>
	class PulseCurve;

	template<typename Time, typename Data>
	class Curve
	{
	public:
		Curve(CurveType type) : _type(type)
		{}

		virtual ~Curve() {}

		//adds a keyframe
		//when you add a keyframe, all keyframes after it are erased
		virtual void set(Time at, Data value)
		{
			auto at = data.insert({ at, value });

			//if the insertion was successful and isn't the last element
			//in the container
			if (at.second && at.first != data.rbegin())
				//erase everything after the newly inserted keyframe
				data.erase(++at.first, data.end());
		}

		virtual Data get(Time at) = 0;

		//For converting to the usable Curve Types
		CurveType type() { return _type; }
		ConstCurve<Time, Data>* asConst() { assert(_type == CurveType::CONST);return this; }
		LinearCurve<Time, Data>* asLinear() { assert(_type == CurveType::LINEAR);return this; }
		StepCurve<Time, Data>* asStep() { assert(_type == CurveType::STEP);return this; }
		PulseCurve<Time, Data>* asPulse() { assert(_type == CurveType::PULSE);return this; }
		
	protected:
		using FrameType = Keyframe<Time, Data>;
		using DataType = std::set< FrameType >;
		using IterPair = std::pair<typename DataType::iterator, typename DataType::iterator>;

		IterPair GetRange(Time at)
		{
			auto next = std::lower_bound(data.begin(), data.end(), at);
			return IterPair(next - 1, next);
		}

		DataType data;

	private:
		CurveType _type;
	};

	//Const Curve
	//There is no actual curve, the value is constant across the entire curve
	//returns the same result no matter what time is requested
	template<typename Time, typename Data>
	class ConstCurve final : public Curve<Time, Data>
	{
	public:
		ConstCurve() : Curve<Time, Data>(CurveType::CONST) {}

		virtual void set(Time at, Data value)
		{
			data.insert({ Time(), value });
		}

		virtual Data get(Time at)
		{
			return data.begin();
		}
	};

	template<typename Time, typename Data>
	class LinearCurve final : public Curve<Time, Data>
	{
	public:
		LinearCurve() : Curve<Time, Data>(CurveType::LINEAR) {}

		virtual Data get(Time at) 
		{
			static_assert(std::is_arithmetic<Data>::value, "Only arithmatic types can be stored in a LinearCurve.");
			//TODO: provide a overridable lerp function for this to fall back on?

			//linear imp goes here
			auto d = GetRange(at);

			//return lerp of d.first to d.second with at.
			assert(false && "unimplemented");

			return Data();
		}
	};

	template<typename Time, typename Data>
	class StepCurve final : public Curve<Time, Data>
	{
	public:
		StepCurve() : Curve<Time, Data>(CurveType::STEP) {}

		virtual Data get(Time at)
		{
			auto d = GetRange(at);

			return *d.first;
		}
	};

	template<typename Time, typename Data>
	class PulseCurve final : public Curve<Time, Data>
	{
	public:
		using DataInfo =  std::pair<Time, Data>;

		PulseCurve() : Curve<Time, Data>(CurveType::PULSE) {}

		virtual Data get(Time at)
		{
			assert(false && "Calling get on Pulse curve doesn't really make sense, use getLast");
			//pulse wont really work with this api will it
		}

		//returns the most recent pulse for that time
		DataInfo getAt(Time at)
		{
			auto d = GetRange(at);

			return d.first;
		}
	};
}

#endif //HADES_CURVES_HPP
