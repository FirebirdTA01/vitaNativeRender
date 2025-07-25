#pragma once

#define PI 3.1415926535897932384626433832795028841971693993751f

struct Vector2f
{
	Vector2f()
	{
		x = 0.0f;
		y = 0.0f;
	}
	Vector2f(float inX, float inY)
	{
		x = inX;
		y = inY;
	}
	float x, y;
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
	//union { float x; float u; };
	//union { float y; float g; };
	//union { float z; float b; };
	float x, y, z;
};

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

struct UnlitColorVertex
{
	UnlitColorVertex() : x(0.0f), y(0.0f), z(0.0f), col() {};
	UnlitColorVertex(float x, float y, float z, Color color) : x(x), y(y), z(z), col(color) {};
	float x, y, z;
	Color col;
};

struct UnlitTexturedVertex
{
	UnlitTexturedVertex() : x(0.0f), y(0.0f), z(0.0f), uv(0.0f, 0.0f) {};
	UnlitTexturedVertex(float x, float y, float z, float u, float v) : x(x), y(y), z(z), uv(u, v) {};
	UnlitTexturedVertex(float x, float y, float z, Vector2f uv) : x(x), y(y), z(z), uv(uv) {};
	float x, y, z;
	Vector2f uv;
};

struct TexturedVertex
{
	TexturedVertex() : x(0.0f), y(0.0f), z(0.0f), uv(0.0f, 0.0f), normal(0.0f, 0.0f, 0.0f) {};
	TexturedVertex(float x, float y, float z, float u, float v, float nx, float ny, float nz) : x(x), y(y), z(z), uv(u, v), normal(nx, ny, nz) {};
	TexturedVertex(float x, float y, float z, Vector2f uv, Vector3f normal) : x(x), y(y), z(z), uv(uv), normal(normal) {};
	float x, y, z;
	Vector2f uv;
	Vector3f normal;
};

struct PBRVertex
{
	PBRVertex() : x(0.0f), y(0.0f), z(0.0f), uv(0.0f, 0.0f), normal(0.0f, 0.0f, 0.0f), tangent(0.0f, 0.0f, 0.0f), bitangent(0.0f, 0.0f, 0.0f) {};
	// This vertex type is only used for the terrain for now, so tangents/bitangents are set here for a flat grid like model with upward facing normals
	PBRVertex(Vector3f inPos, Vector2f inUV, Vector3f inNormal) : x(inPos.x), y(inPos.y), z(inPos.z), uv(inUV), normal(inNormal), tangent(1.0f, 0.0f, 0.0f), bitangent(0.0f, 0.0f, 1.0f) {};
	float x, y, z;
	Vector2f uv;
	Vector3f normal;
	Vector3f tangent;
	Vector3f bitangent;
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
