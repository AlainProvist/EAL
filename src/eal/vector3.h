/* 
 * This file is part of the Eidolon Auto Link distribution (https://github.com/AlainProvist/EAL).
 * Copyright (c) 2019 AlainProvist.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef VECTOR3_H
#define VECTOR3_H

#include <math.h>

#define Epsilon 0.00001f
#define EqualWithEpsilon(a,b) (((a) + Epsilon >= (b)) && ((a) - Epsilon <= (b)))
#define NullWithEpsilon(a) (((a) + Epsilon >= 0) && ((a) - Epsilon <= 0))

template <class T>
class Vector3
{
public:     // interface
    //! Default constructor (null vector).
	Vector3() {m_Data[0] = m_Data[1] = m_Data[2] = (T)0;}
	//! Constructor with three different values
	Vector3(T nx, T ny, T nz) {m_Data[0] = nx; m_Data[1] = ny; m_Data[2] = nz;}
	//! Constructor with the same value for all elements
	explicit Vector3(T n) {m_Data[0] = m_Data[1] = m_Data[2] = n;}
	//! Copy constructor
	Vector3(const Vector3<T>& other) {m_Data[0] = other.X(); m_Data[1] = other.Y(); m_Data[2] = other.Z();}

    //
    //  operators

    // operator: indexing
    const T&    operator[] (int i) const    { return m_Data[i]; }
    T&          operator[] (int i)      { return m_Data[i]; }

    // operators: math
	Vector3<T> operator-() const { return Vector3<T>(-X(), -Y(), -Z()); }

	Vector3<T>& operator=(const Vector3<T>& other) { X() = other.X(); Y() = other.Y(); Z() = other.Z(); return *this; }

	Vector3<T> operator+(const Vector3<T>& other) const { return Vector3<T>(X() + other.X(), Y() + other.Y(), Z() + other.Z()); }
	Vector3<T>& operator+=(const Vector3<T>& other) { X()+=other.X(); Y()+=other.Y(); Z()+=other.Z(); return *this; }
	Vector3<T> operator+(const T val) const { return Vector3<T>(X() + val, Y() + val, Z() + val); }
	Vector3<T>& operator+=(const T val) { X()+=val; Y()+=val; Z()+=val; return *this; }

	Vector3<T> operator-(const Vector3<T>& other) const { return Vector3<T>(X() - other.X(), Y() - other.Y(), Z() - other.Z()); }
	Vector3<T>& operator-=(const Vector3<T>& other) { X()-=other.X(); Y()-=other.Y(); Z()-=other.Z(); return *this; }
	Vector3<T> operator-(const T val) const { return Vector3<T>(X() - val, Y() - val, Z() - val); }
	Vector3<T>& operator-=(const T val) { X()-=val; Y()-=val; Z()-=val; return *this; }

	Vector3<T> operator*(const Vector3<T>& other) const { return Vector3<T>(X() * other.X(), Y() * other.Y(), Z() * other.Z()); }
	Vector3<T>& operator*=(const Vector3<T>& other) { X()*=other.X(); Y()*=other.Y(); Z()*=other.Z(); return *this; }
	Vector3<T> operator*(const T v) const { return Vector3<T>(X() * v, Y() * v, Z() * v); }
	Vector3<T>& operator*=(const T v) { X()*=v; Y()*=v; Z()*=v; return *this; }

	Vector3<T> operator/(const Vector3<T>& other) const { return Vector3<T>(X() / other.X(), Y() / other.Y(), Z() / other.Z()); }
	Vector3<T>& operator/=(const Vector3<T>& other) { X()/=other.X(); Y()/=other.Y(); Z()/=other.Z(); return *this; }
	Vector3<T> operator/(const T v) const { T i=(T)1.0/v; return Vector3<T>(X() * i, Y() * i, Z() * i); }
	Vector3<T>& operator/=(const T v) { T i=(T)1.0/v; X()*=i; Y()*=i; Z()*=i; return *this; }

	// sort in order X, Y, Z. Equality with rounding tolerance.
	bool operator<=(const Vector3<T>& other) const
	{
		return 	(X()<other.X() || EqualWithEpsilon(X(), other.X())) ||
			(EqualWithEpsilon(X(), other.X()) && (Y()<other.Y() || EqualWithEpsilon(Y(), other.Y()))) ||
			(EqualWithEpsilon(X(), other.X()) && EqualWithEpsilon(Y(), other.Y()) && (Z()<other.Z() || EqualWithEpsilon(Z(), other.Z())));
	}

	// sort in order X, Y, Z. Equality with rounding tolerance.
	bool operator>=(const Vector3<T>&other) const
	{
		return 	(X()>other.X() || EqualWithEpsilon(X(), other.X())) ||
			(EqualWithEpsilon(X(), other.X()) && (Y()>other.Y() || EqualWithEpsilon(Y(), other.Y()))) ||
			(EqualWithEpsilon(X(), other.X()) && EqualWithEpsilon(Y(), other.Y()) && (Z()>other.Z() || EqualWithEpsilon(Z(), other.Z())));
	}

	// sort in order X, Y, Z. Difference must be above rounding tolerance.
	bool operator<(const Vector3<T>&other) const
	{
		return 	(X()<other.X() && !EqualWithEpsilon(X(), other.X())) ||
			(EqualWithEpsilon(X(), other.X()) && Y()<other.Y() && !EqualWithEpsilon(Y(), other.Y())) ||
			(EqualWithEpsilon(X(), other.X()) && EqualWithEpsilon(Y(), other.Y()) && Z()<other.Z() && !EqualWithEpsilon(Z(), other.Z()));
	}

	// sort in order X, Y, Z. Difference must be above rounding tolerance.
	bool operator>(const Vector3<T>&other) const
	{
		return 	(X()>other.X() && !EqualWithEpsilon(X(), other.X())) ||
			(EqualWithEpsilon(X(), other.X()) && Y()>other.Y() && !EqualWithEpsilon(Y(), other.Y())) ||
			(EqualWithEpsilon(X(), other.X()) && EqualWithEpsilon(Y(), other.Y()) && Z()>other.Z() && !EqualWithEpsilon(Z(), other.Z()));
	}

	// use weak float compare
	bool operator==(const Vector3<T>& other) const
	{
		return this->equals(other);
	}

	bool operator!=(const Vector3<T>& other) const
	{
		return !this->equals(other);
	}

	// functions

	//! returns if this vector equals the other one, taking floating point rounding errors into account
	bool equals(const Vector3<T>& other) const
	{
		return EqualWithEpsilon(X(), other.X()) &&
			EqualWithEpsilon(Y(), other.Y()) &&
			EqualWithEpsilon(Z(), other.Z());
	}

    //
    // named functions

    // Set Value
    void Set(T x, T y, T z);

    // return length of vector
    float       GetLength() const;

	// return length 2D of vector
	float       GetLength2() const;

    // return length of vector squared
    float       GetSquareLength() const;

	// return length 2D of vector squared
	float       GetSquareLength2() const;

    // normalize a vector
    void        Normalize();

    // perform dot product
    T           Dot(const Vector3<T>&) const;

    // perform cross product(same as operator*=)
    Vector3<T>   Cross(const Vector3<T>&) const;

    // accessor functions
    T&          X()         { return m_Data[0]; }
    const T&    X() const   { return m_Data[0]; }
    T&          Y()         { return m_Data[1]; }
    const T&    Y() const   { return m_Data[1]; }
    T&          Z()         { return m_Data[2]; }
    const T&    Z() const   { return m_Data[2]; }

    const T*    GetData() const    { return m_Data; }

	// static usefull methods
	static const Vector3<T>& GetZero() {return ms_Zero;}
	static const Vector3<T>& GetBaseI() {return ms_BaseI;}
	static const Vector3<T>& GetBaseJ() {return ms_BaseJ;}
	static const Vector3<T>& GetBaseK() {return ms_BaseK;}

private:
    T m_Data[3];

	// static usefull vectors
	static Vector3<T> ms_Zero;
	static Vector3<T> ms_BaseI;
	static Vector3<T> ms_BaseJ;
	static Vector3<T> ms_BaseK;
};
typedef Vector3<float> Vector3f;

template <class T> Vector3<T> Vector3<T>::ms_Zero = Vector3<T>((T)0);
template <class T> Vector3<T> Vector3<T>::ms_BaseI = Vector3<T>((T)1, (T)0, (T)0);
template <class T> Vector3<T> Vector3<T>::ms_BaseJ = Vector3<T>((T)0, (T)1, (T)0);
template <class T> Vector3<T> Vector3<T>::ms_BaseK = Vector3<T>((T)0, (T)0, (T)1);

template <class T> inline T
Vector3<T>::Dot(const Vector3<T>& v) const
{
    return (T)(X() * v.X() + Y() * v.Y() + Z() * v.Z());
}

template <class T> inline Vector3<T>
Vector3<T>::Cross(const Vector3<T>& v) const
{
	return Vector3<T> (  Y() * v.Z() - Z() * v.Y(),
                          -(X() * v.Z() - Z() * v.X()),
                          X() * v.Y() - Y() * v.X());
}

template <class T> inline void
Vector3<T>::Set(T x, T y, T z) 
{
    m_Data[0] = x;
    m_Data[1] = y;
    m_Data[2] = z;
}

template <class T> inline float
Vector3<T>::GetLength() const
{
    return ::sqrt(GetSquareLength());
}


template <class T> inline float
Vector3<T>::GetSquareLength() const
{
    return (X() * X() + Y() * Y() + Z() * Z());
}

template <class T> inline float
Vector3<T>::GetLength2() const
{
	return ::sqrt(GetSquareLength2());
}


template <class T> inline float
Vector3<T>::GetSquareLength2() const
{
	return (X() * X() + Y() * Y());
}

template <class T> inline void
Vector3<T>::Normalize()
{
    float len = GetLength();

    if (len != 0) 
	{
        float f = 1.0f / len;

        *this *= f;
    }
}

#endif // VECTOR3_H