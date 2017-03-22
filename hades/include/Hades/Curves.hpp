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
		CONST, //data is constant for any keyframe
		LINEAR, //data between keyframes is exactly the difference between them
		STEP, // data between keyframes is identical to the previous keyframe
		PULSE // data between keyframes is null
	};

	template<typename Time, typename Data>
	class Curve {
	public:
		Curve()
		{
			static_assert(hades::types::hades_type<Data>::value, "Curves can only contain approved data types");
		}

		virtual Data get(Time at) = 0;
	protected:
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
	class ConstCurve final : public Curve<Time, Data>
	{
	public:
		void set(Data value)
		{
			data = value;
		}

		virtual Data get(Time at)
		{
			return data;
		}

	private:
		Data data;
	};

	template<typename Time, typename Data>
	class LinearCurve final : public Curve<Time, Data>
	{
	public:
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
		typedef std::pair<Time, Data> DataInfo;

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
