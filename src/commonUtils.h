#pragma once

#define PI 3.1415926535897932384626433832795028841971693993751f

struct Color
{
	Color() : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {};
	Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {};
	float r, g, b, a;
};

struct ClearVertex
{
	ClearVertex() : x(0.0f), y(0.0f) {};
	ClearVertex(float x, float y) : x(x), y(y) {};
	float x, y;
};

struct UnlitColorVertex {
	UnlitColorVertex() : x(0.0f), y(0.0f), z(0.0f), col() {};
	UnlitColorVertex(float x, float y, float z, Color color) : x(x), y(y), z(z), col(color) {};
	float x, y, z;
	Color col;
};

struct Vector3f
{
	Vector3f()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}
	Vector3f(float inX, float inY, float inZ)
	{
		x = inX;
		y = inY;
		z = inZ;
	}
	//union { float x; float r; };
	//union { float y; float g; };
	//union { float z; float b; };
	float x, y, z;
};


inline Vector3f operator+(const Vector3f& left, const Vector3f& right)
{
	return Vector3f(left.x + right.x, left.y + right.y, left.z + right.z);
}

inline Vector3f operator-(const Vector3f& left, const Vector3f& right)
{
	return Vector3f(left.x - right.x, left.y - right.y, left.z - right.z);
}

inline Vector3f operator*(const Vector3f& left, const Vector3f& right)
{
	return Vector3f(left.x * right.x, left.y * right.y, left.z * right.z);
}

inline Vector3f operator/(const Vector3f& left, const Vector3f& right)
{
	return Vector3f(left.x / right.x, left.y / right.y, left.z / right.z);
}

inline Vector3f operator*(const Vector3f& left, float right)
{
	return Vector3f(left.x * right, left.y * right, left.z * right);
}

inline Vector3f operator/(const Vector3f& left, float right)
{
	return Vector3f(left.x / right, left.y / right, left.z / right);
}

inline Vector3f& operator+=(Vector3f& left, const Vector3f& right)
{
	left.x += right.x;
	left.y += right.y;
	left.z += right.z;

	return left;
}

inline Vector3f& operator-=(Vector3f& left, const Vector3f& right)
{
	left.x -= right.x;
	left.y -= right.y;
	left.z -= right.z;

	return left;
}
