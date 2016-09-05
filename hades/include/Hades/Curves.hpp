#ifndef HADES_CURVES_HPP
#define HADES_CURVES_HPP

#include <algorithm>
#include <set>

#include "Types.hpp"

//A set of curve classes for single player games

namespace hades {
	template<typename Time, typename Data>
	struct Keyframe
	{
		Keyframe(Time at) : t(at) {}

		Time t;
		Data value;
	};

	enum CurveType {
		LINEAR, //data between keyframes is exactly the difference between them
		STEP, // data between keyframes is identical to the previous keyframe
		PULSE // data between keyframes is null
	};

	template<typename Time, typename Data>
	class Curve {
	public:
		Curve()
		{
			static_assert(hades::types::hades_type<Data>::value, "Curves can only contain syncronisable data");
		}

		Data Get(Time at) = 0;
	private:
		typedef std::set<Keyframe> DataType;
		typedef std::pair<DataType::iterator, DataType::iterator> IterPair;

		IterPair GetRange(Time at)
		{
			auto next = std::lower_bound(data.begin(), data.end(), at);
			return IterPair(next - 1, next);
		}

		DataType data;
	};

	template<typename Time, typename Data>
	class LinearCurve : public Curve<Time, Data>
	{
	public:
		Data Get(Time at) 
		{
			//linear imp goes here
			auto d = GetRange(at);

			//return lerp of d.first to d.second with at.
			assert(false && "unimplemented");

			return Data();
		}
	};

	template<typename Time, typename Data>
	class StepCurve : public Curve<Time, Data>
	{
	public:
		Data Get(Time at)
		{
			auto d = GetRange(at);

			return *d.first;
		}
	};

	template<typename Time, typename Data>
	class PulseCurve : public Curve<Time, Data>
	{
	public:
		typedef std::pair<Time, Data> DataInfo;

		Data Get(Time at)
		{
			assert(false && "Calling Get on Pulse curve doesn't really make sense, use GetLast");
			//pulse wont really work with this api will it
		}

		DataInfo GetLast(Time at)
		{
			auto d = GetRange(at);

			return d.first;
		}
	};
}

#endif //HADES_CURVES_HPP
