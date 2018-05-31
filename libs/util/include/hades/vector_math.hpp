#ifndef HADES_UTIL_VECTOR_MATH_HPP
#define HADES_UTIL_VECTOR_MATH_HPP

#include <optional>
#include <tuple>

namespace hades
{
	template<typename T>
	struct vector_t
	{
		T x, y;
	};

	template<typename T>
	vector_t<T> operator+(const vector_t<T> &lhs, const vector_t<T> &rhs);

	template<typename T>
	vector_t<T> operator-(const vector_t<T> &lhs, const vector_t<T> &rhs);

	template<typename T>
	vector_t<T> operator*(const vector_t<T> &lhs, T rhs);

	template<typename T>
	vector_t<T> operator/(const vector_t<T> &lhs, T rhs);

	template<typename T>
	struct rad_vector_t
	{
		T a, m;
	};

	template<typename T>
	vector_t<T> to_vector(rad_vector_t<T> v);

	template<typename T>
	rad_vector_t<T> to_rad_vector(vector_t<T>);

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
		vector_t<T> unit(vector_t<T>);

		template<typename T>
		T distance(vector_t<T>, vector_t<T>);

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

#endif // !HADES_UTIL_VECTOR_MATH_HPP