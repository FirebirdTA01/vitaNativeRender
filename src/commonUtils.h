#pragma once

#include <cmath>
#include <cstdint>

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

	float length() const
	{
		return sqrtf(x * x + y * y);
	}

	float lengthSquared() const
	{
		return x * x + y * y;
	}

	Vector2f normalized() const
	{
		float len = length();
		return len > 0 ? Vector2f(x / len, y / len) : Vector2f(0.0f, 0.0f);
	}

	float dot(const Vector2f& op) const
	{
		return x * op.x + y * op.y;
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

	float length() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	float lengthSquared() const
	{
		return x * x + y * y + z * z;
	}

	Vector3f normalized() const
	{
		float len = length();
		return len > 0 ? Vector3f(x / len, y / len, z / len) : Vector3f(0.0f, 0.0f, 0.0f);
	}

	float dot(const Vector3f& op) const
	{
		return x * op.x + y * op.y + z * op.z;
	}

	//union { float x; float u; };
	//union { float y; float g; };
	//union { float z; float b; };
	float x, y, z;
};

struct Vector4f
{
	Vector4f()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 0.0f;
	}
	Vector4f(float inX, float inY, float inZ, float inW)
	{
		x = inX;
		y = inY;
		z = inZ;
		w = inW;
	}

	float x, y, z, w;
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

// Compact terrain vertex: position + UV only (20 bytes vs 56 for PBRVertex)
// Normal, tangent, bitangent are hardcoded in the terrain vertex shader since terrain is flat
struct TerrainVertex
{
	TerrainVertex() : x(0.0f), y(0.0f), z(0.0f), u(0.0f), v(0.0f) {};
	TerrainVertex(float x, float y, float z, float u, float v) : x(x), y(y), z(z), u(u), v(v) {};
	TerrainVertex(Vector3f pos, Vector2f uv) : x(pos.x), y(pos.y), z(pos.z), u(uv.x), v(uv.y) {};
	float x, y, z;  // 12 bytes
	float u, v;      // 8 bytes
};                   // 20 bytes total

// Helper to convert float [-1,1] to signed 16-bit normalized
inline int16_t floatToS16N(float v)
{
	if (v < -1.0f) v = -1.0f;
	if (v > 1.0f) v = 1.0f;
	return (int16_t)(v * 32767.0f);
}

// Compressed terrain PBR vertex: position + UV as float32, normal + tangent as signed 16-bit normalized
// Handedness is precomputed on CPU and stored in tangent.w, eliminating bitangent from vertex data
// S16N provides full precision for normalized vectors in the -1 to 1 range
struct TerrainPBRVertex
{
	TerrainPBRVertex() : x(0.0f), y(0.0f), z(0.0f), u(0.0f), v(0.0f),
		nx(0), ny(0), nz(0), tx(0), ty(0), tz(0), tw(0), _pad(0) {};

	TerrainPBRVertex(Vector3f inPos, Vector2f inUV, Vector3f inNormal,
		Vector3f inTangent, Vector3f inBitangent)
		: x(inPos.x), y(inPos.y), z(inPos.z), u(inUV.x), v(inUV.y),
		  nx(floatToS16N(inNormal.x)), ny(floatToS16N(inNormal.y)), nz(floatToS16N(inNormal.z)),
		  tx(floatToS16N(inTangent.x)), ty(floatToS16N(inTangent.y)), tz(floatToS16N(inTangent.z)),
		  _pad(0)
	{
		// Compute handedness: +1 if (NxT)·B > 0, else -1
		Vector3f c = {
			inNormal.y * inTangent.z - inNormal.z * inTangent.y,
			inNormal.z * inTangent.x - inNormal.x * inTangent.z,
			inNormal.x * inTangent.y - inNormal.y * inTangent.x
		};
		float d = c.x * inBitangent.x + c.y * inBitangent.y + c.z * inBitangent.z;
		tw = floatToS16N(d < 0.0f ? -1.0f : 1.0f);
	};

	float x, y, z;              // 12 bytes - position (full precision)
	float u, v;                 // 8 bytes - UV (full precision for tiling)
	int16_t nx, ny, nz;         // 6 bytes - normal (S16N)
	int16_t tx, ty, tz;         // 6 bytes - tangent xyz (S16N)
	int16_t tw;                 // 2 bytes - tangent w = handedness sign (S16N)
	int16_t _pad;               // 2 bytes - padding for 4-byte alignment
};                              // 36 bytes total

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

inline Vector4f operator+(const Vector4f& left, const Vector4f& right)
{
	return Vector4f(left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w);
}

inline Vector4f operator-(const Vector4f& left, const Vector4f& right)
{
	return Vector4f(left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w);
}

inline Vector4f operator*(const Vector4f& left, const Vector4f& right)
{
	return Vector4f(left.x * right.x, left.y * right.y, left.z * right.z, left.w * right.w);
}

inline Vector4f operator/(const Vector4f& left, const Vector4f& right)
{
	return Vector4f(left.x / right.x, left.y / right.y, left.z / right.z, left.w / right.w);
}

inline Vector4f operator*(const Vector4f& left, float right)
{
	return Vector4f(left.x * right, left.y * right, left.z * right, left.w * right);
}

inline Vector4f operator/(const Vector4f& left, float right)
{
	return Vector4f(left.x / right, left.y / right, left.z / right, left.w / right);
}

inline Vector4f& operator+=(Vector4f& left, const Vector4f& right)
{
	left.x += right.x;
	left.y += right.y;
	left.z += right.z;
	left.w += right.w;

	return left;
}

inline Vector4f& operator-=(Vector4f& left, const Vector4f& right)
{
	left.x -= right.x;
	left.y -= right.y;
	left.z -= right.z;
	left.w -= right.w;

	return left;
}