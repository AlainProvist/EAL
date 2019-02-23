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


#ifndef MATRIX3_H
#define MATRIX3_H

#include "Vector3.h"


template <class T>
class Matrix3
{
public:
	// ctors
	Matrix3();
	Matrix3 (T,T,T, T,T,T, T,T,T);
	Matrix3 (const T[9]);
	Matrix3 (const T[3][3]);

	//
	// operators
	//

	// operator: index with bounds checking
	T           operator() (int i, int j) const { return m_Data[j * 3+i]; }
	T&          operator() (int i, int j)       { return m_Data[j * 3+i]; }

	// operator: index with no bounds checking
	T           operator[] (int i) const    { return m_Data[i]; }
	T&          operator[] (int i)          { return m_Data[i]; }

	// operator: addition
	Matrix3<T>& operator+= (T);
	Matrix3<T>& operator+= (const Matrix3<T>&);
	Matrix3<T> operator+(const Matrix3<T>& other) const;

	// operator: subtraction
	Matrix3<T>& operator-= (T);
	Matrix3<T>& operator-= (const Matrix3<T>&);
	Matrix3<T> operator-(const Matrix3<T>& other) const;

	// operator: multiplication
	Matrix3<T>& operator*= (T);
	Matrix3<T>& operator*= (const Matrix3<T>&);
	Matrix3<T> operator*(const Matrix3<T>& other) const;

	// operator: division
	Matrix3<T>& operator/= (T);

	//
	// named functions

	// transpose the current matrix
	void        Transpose();

	// make the current matrix an identity matrix
	void        Identity();

	// clear the current matrix to a given value
	void        Clear(T x = (T)0);

	// return the determinant of the current matrix
	T           Determinant() const;

	// data accessor
	const T*    GetData() const    { return m_Data; }
	T*    GetData()    { return m_Data; }

	// return a vector representing a row
	Vector3<T>   Row(int r) const;

	// return a vector representing a column
	Vector3<T>   Column(int c) const;

	Vector3<T>   GetHorizon() const;
	Vector3<T>   GetSight() const;
	Vector3<T>   GetBanking() const;

	void SetOrientation(const Vector3<T>& sight, const Vector3<T>& banking = Vector3<T>((T)0, (T)0, (T)1));

	// static usefull methods
	static const Matrix3<T>& GetZero() {return ms_Zero;}
	static const Matrix3<T>& GetIdentity() {return ms_Identity;}

private:
	T m_Data[9];

	// static usefull matrix
	static Matrix3<T> ms_Zero;
	static Matrix3<T> ms_Identity;

};
typedef Matrix3<float> Matrix3f;

template <class T> Matrix3<T> Matrix3<T>::ms_Zero = Matrix3<T>((T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0, (T)0);
template <class T> Matrix3<T> Matrix3<T>::ms_Identity = Matrix3<T>((T)1, (T)0, (T)0, (T)0, (T)1, (T)0, (T)0, (T)0, (T)1);

//
// default ctor
//
template <class T> inline
Matrix3<T>::Matrix3()
{
	Clear(T(0));
}

//
// conversion ctors
//
template <class T> inline
Matrix3<T>::Matrix3 (T m1, T m2, T m3,  T m4, T m5, T m6,  T m7, T m8, T m9)
{
	m_Data[0] = m1; m_Data[1] = m2; m_Data[2] = m3;
	m_Data[3] = m4; m_Data[4] = m5; m_Data[5] = m6;
	m_Data[6] = m7; m_Data[7] = m8; m_Data[8] = m9;
}


template <class T> inline
Matrix3<T>::Matrix3(const T m[9])
{
	for (int i = 0;  i < 9;  ++i) 
	{
		m_Data[i] = m[i];
	}
}


template <class T> inline
Matrix3<T>::Matrix3(const T m[3][3])
{
	m_Data[0] = m[0][0];
	m_Data[1] = m[0][1];
	m_Data[2] = m[0][2];
	m_Data[3] = m[1][0];
	m_Data[4] = m[1][1];
	m_Data[5] = m[1][2];
	m_Data[6] = m[2][0];
	m_Data[7] = m[2][1];
	m_Data[8] = m[2][2];
}


template <class T> inline Matrix3<T>&
Matrix3<T>::operator+= (const Matrix3<T>& m)
{
	for (int i = 0;  i < 9;  ++i) 
	{
		m_Data[i] += m[i];
	}

	return *this;
}

template <class T> inline Matrix3<T>&
Matrix3<T>::operator+= (T scalar)
{
	for (int i = 0;  i < 9;  ++i) 
	{
		m_Data[i] += scalar;
	}

	return *this;
}

template <class T> inline Matrix3<T> 
Matrix3<T>::operator+(const Matrix3<T>& other) const
{
	Matrix3<T> temp();

	for (int i = 0;  i < 9;  ++i) 
	{
		m_Data[i] = m_Data[i]+other[i];
	}

	return temp;
}

template <class T> inline Matrix3<T>&
Matrix3<T>::operator-= (const Matrix3<T>& m)
{
	for (int i = 0;  i < 9;  ++i) {
		m_Data[i] -= m[i];
	}

	return *this;
}

template <class T> inline Matrix3<T>&
Matrix3<T>::operator-= (T scalar)
{
	for (int i = 0;  i < 9;  ++i) {
		m_Data[i] -= scalar;
	}

	return *this;
}

template <class T> inline Matrix3<T> 
Matrix3<T>::operator-(const Matrix3<T>& other) const
{
	Matrix3<T> temp();

	for (int i = 0;  i < 9;  ++i) 
	{
		m_Data[i] = m_Data[i]-other[i];
	}

	return temp;
}


template <class T> inline Matrix3<T>&
Matrix3<T>::operator*= (T scalar)
{
	for (int i = 0;  i < 9;  ++i) {
		m_Data[i] *= scalar;
	}

	return *this;
}

template <class T> inline Matrix3<T>&
Matrix3<T>::operator*= (const Matrix3<T>& m)
{
	T f1, f2, f3;

	f1 = m_Data[0]*m[0] + m_Data[3]*m[1] + m_Data[6]*m[2];
	f2 = m_Data[1]*m[0] + m_Data[4]*m[1] + m_Data[7]*m[2];
	f3 = m_Data[2]*m[0] + m_Data[5]*m[1] + m_Data[8]*m[2];
	m_Data[0] = f1;
	m_Data[3] = f2;
	m_Data[6] = f3;

	f1 = m_Data[0]*m[3] + m_Data[3]*m[4] + m_Data[6]*m[5];
	f2 = m_Data[1]*m[3] + m_Data[4]*m[4] + m_Data[7]*m[5];
	f3 = m_Data[2]*m[3] + m_Data[5]*m[4] + m_Data[8]*m[5];
	m_Data[1] = f1;
	m_Data[4] = f2;
	m_Data[7] = f3;

	f1 = m_Data[0]*m[6] + m_Data[3]*m[7] + m_Data[6]*m[8];
	f2 = m_Data[1]*m[6] + m_Data[4]*m[7] + m_Data[7]*m[8];
	f3 = m_Data[2]*m[6] + m_Data[5]*m[7] + m_Data[8]*m[8];
	m_Data[2] = f1;
	m_Data[5] = f2;
	m_Data[8] = f3;

	return *this;
}

template <class T> inline Matrix3<T> 
Matrix3<T>::operator*(const Matrix3<T>& m2) const
{
	Matrix3<T> m3();

	const T *m1 = m_Data;

	m3[0] = m1[0]*m2[0] + m1[3]*m2[1] + m1[6]*m2[2];
	m3[3] = m1[1]*m2[0] + m1[4]*m2[1] + m1[7]*m2[2];
	m3[6] = m1[2]*m2[0] + m1[5]*m2[1] + m1[8]*m2[2];

	m3[1] = m1[0]*m2[3] + m1[3]*m2[4] + m1[6]*m2[5];
	m3[4] = m1[1]*m2[3] + m1[4]*m2[4] + m1[7]*m2[5];
	m3[7] = m1[2]*m2[3] + m1[5]*m2[4] + m1[8]*m2[5];

	m3[2] = m1[0]*m2[6] + m1[3]*m2[7] + m1[6]*m2[8];
	m3[5] = m1[1]*m2[6] + m1[4]*m2[7] + m1[7]*m2[8];
	m3[8] = m1[2]*m2[6] + m1[5]*m2[7] + m1[8]*m2[8];

	return m3;
}

template <class T> inline Matrix3<T>&
Matrix3<T>::operator/= (T scalar)
{
	scalar = T(1) / scalar;
	for (int i = 0;  i < 9;  ++i) 
	{
		m_Data[i] *= scalar;
	}

	return *this;
}


template <class T> inline void
Matrix3<T>::Transpose()
{
	std::swap(m_Data[3], m_Data[1]);
	std::swap(m_Data[6], m_Data[2]);
	std::swap(m_Data[7], m_Data[5]);
}

template <class T> inline void
Matrix3<T>::Identity()
{
	m_Data[0] = (T)1;
	m_Data[1] = (T)0;
	m_Data[2] = (T)0;

	m_Data[3] = (T)0;
	m_Data[4] = (T)1;
	m_Data[5] = (T)0;

	m_Data[6] = (T)0;
	m_Data[7] = (T)0;
	m_Data[8] = (T)1;
}

template <class T> inline void
Matrix3<T>::Clear(T value)
{
	for (int i = 0;  i < 9;  ++i) {
		m_Data[i] = value;
	}
}


template <class T> inline Vector3<T>
Matrix3<T>::Row(int i) const
{
	return Vector3<T> (m_Data[i], m_Data[3 + i], m_Data[6 + i]);
}


template <class T> inline Vector3<T>
Matrix3<T>::Column(int j) const
{
	return Vector3<T> (m_Data[j * 3], m_Data[j * 3 + 1], m_Data[j * 3 + 2]);
}


template <class T> inline T
Matrix3<T>::Determinant() const
{
	return (m_Data[0] * (m_Data[4]*m_Data[8] - m_Data[5]*m_Data[7]) +
		m_Data[1] * (m_Data[3]*m_Data[8] - m_Data[5]*m_Data[6]) +
		m_Data[2] * (m_Data[3]*m_Data[7] - m_Data[4]*m_Data[6]));
}

template <class T> inline Vector3<T>
Matrix3<T>::GetHorizon() const
{
	return Column(0);
}

template <class T> inline Vector3<T>
Matrix3<T>::GetSight() const
{
	return Column(1);
}

template <class T> inline Vector3<T>
Matrix3<T>::GetBanking() const
{
	return Column(2);
}

template <class T> inline void
Matrix3<T>::SetOrientation(const Vector3<T>& sight, const Vector3<T>& banking/* = Vector3<T>((T)0, (T)0, (T)1)*/)
{
	Vector3<T> horizon = sight.Cross(banking);
	Vector3<T> realSight = banking.Cross(horizon);

	m_Data[0] = horizon.X();
	m_Data[1] = horizon.Y();
	m_Data[2] = horizon.Z();

	m_Data[3] = realSight.X();
	m_Data[4] = realSight.Y();
	m_Data[5] = realSight.Z();

	m_Data[6] = banking.X();
	m_Data[7] = banking.Y();
	m_Data[8] = banking.Z();
}

#endif // MATRIX3_H