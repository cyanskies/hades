#ifndef HADES_UTIL_MATH_HPP
#define HADES_UTIL_MATH_HPP

#include <tuple>

namespace hades
{
	template<typename T>
	struct vector_t
	{
		T x, y;
	};

	template<typename T>
	struct rad_vector_t
	{
		T a, m;
	};

	template<typename T>
	vector_t<T> to_vector(rad_vector_t<T>);

	template<typename T>
	rad_vector_t<T> to_rad_vector(rvector_t<T>);

	namespace vector
	{
		//returns the length of the vector
		template<typename T>
		T magnitude(vector_t<T>);

		//returns the angle of the vector
		template<typename T>
		T angle(vector_t<T>);

		template<typename T>
		T x_comp(rad_vector_t<T>);

		template<typename T>
		T y_comp(rad_vector_t<T>);

		//changes the length of a vector to match the provided length
		template<typename T>
		vector_t<T> resize(vector_t<T>, T length);

		template<typename T>
		vector_t<T> unit(T x, T y);

		//returns a vector that points 90 degrees of the origional vector
		template<typename T>
		vector_t<T> perpendicular(vector_t<T>);

		//returns a vector that points 280 degrees of the origional vector
		template<typename T>
		vector_t<T> perpendicular_reverse(vector_t<T>);

		template<typename T>
		vector_t<T> clamp(vector_t<T> value, vector_t<T> min, vector_t<T> max);
	}
}

#include "hades/detail/vector_math.inl"

#endif // !HADES_UTIL_MATH_HPP