#pragma once

#include "commonUtils.h"
#include <array>
#include <cstring> // For memcpy

typedef struct Matrix4x4
{
	Matrix4x4();
	Matrix4x4(std::array<float, 16> inMatrix);
	inline Matrix4x4(const Matrix4x4& rhs)
	{
		std::memcpy(matrix, rhs.matrix, 16 * sizeof(float));
	}

	inline float* getData() { return matrix; }
	inline const float* getData() const { return matrix; }

	//Sets the matrix to the identity matrix. THIS WILL OVERWRITE ANY DATA IN THE MATRIX
	inline void setToIdentity();

	//math
	void translate(Vector3f translation);
	void rotateX(float angle);
	void rotateY(float angle);
	void rotateZ(float angle);
	void rotate(Vector3f angles);
	void scale(Vector3f scale);
	void scale(float scale);

private:
	//Elements are stored in ROW-MAJOR order
	alignas(16) float matrix[16];
public:


	//operators
	inline Matrix4x4& operator=(const Matrix4x4& rhs)
	{
		std::memcpy(matrix, rhs.matrix, 16 * sizeof(float));
		return *this;
	}

	inline Matrix4x4& operator*=(const Matrix4x4& rhs)
	{
		float m00 = matrix[0], m01 = matrix[1], m02 = matrix[2], m03 = matrix[3];
		float m10 = matrix[4], m11 = matrix[5], m12 = matrix[6], m13 = matrix[7];
		float m20 = matrix[8], m21 = matrix[9], m22 = matrix[10], m23 = matrix[11];
		float m30 = matrix[12], m31 = matrix[13], m32 = matrix[14], m33 = matrix[15];

		alignas(16) float r[16];
		std::memcpy(r, rhs.matrix, 16 * sizeof(float));

		//Row 0
		matrix[0] = m00 * r[0] + m01 * r[4] + m02 * r[8] + m03 * r[12];
		matrix[1] = m00 * r[1] + m01 * r[5] + m02 * r[9] + m03 * r[13];
		matrix[2] = m00 * r[2] + m01 * r[6] + m02 * r[10] + m03 * r[14];
		matrix[3] = m00 * r[3] + m01 * r[7] + m02 * r[11] + m03 * r[15];

		//Row 1
		matrix[4] = m10 * r[0] + m11 * r[4] + m12 * r[8] + m13 * r[12];
		matrix[5] = m10 * r[1] + m11 * r[5] + m12 * r[9] + m13 * r[13];
		matrix[6] = m10 * r[2] + m11 * r[6] + m12 * r[10] + m13 * r[14];
		matrix[7] = m10 * r[3] + m11 * r[7] + m12 * r[11] + m13 * r[15];

		//Row 2
		matrix[8] = m20 * r[0] + m21 * r[4] + m22 * r[8] + m23 * r[12];
		matrix[9] = m20 * r[1] + m21 * r[5] + m22 * r[9] + m23 * r[13];
		matrix[10] = m20 * r[2] + m21 * r[6] + m22 * r[10] + m23 * r[14];
		matrix[11] = m20 * r[3] + m21 * r[7] + m22 * r[11] + m23 * r[15];

		//Row 3
		matrix[12] = m30 * r[0] + m31 * r[4] + m32 * r[8] + m33 * r[12];
		matrix[13] = m30 * r[1] + m31 * r[5] + m32 * r[9] + m33 * r[13];
		matrix[14] = m30 * r[2] + m31 * r[6] + m32 * r[10] + m33 * r[14];
		matrix[15] = m30 * r[3] + m31 * r[7] + m32 * r[11] + m33 * r[15];

		return *this;
	}

	inline Matrix4x4& operator*=(const float& rhs)
	{
		for (int i = 0; i < 16; i++)
		{
			matrix[i] *= rhs;
		}
		return *this;
	}

	inline Matrix4x4& operator+=(const Matrix4x4& rhs)
	{
		for (int i = 0; i < 16; i++)
		{
			matrix[i] += rhs.matrix[i];
		}
		return *this;
	}

	inline Matrix4x4& operator-=(const Matrix4x4& rhs)
	{
		for (int i = 0; i < 16; i++)
		{
			matrix[i] -= rhs.matrix[i];
		}
		return *this;
	}

}Matrix4x4;

inline Matrix4x4 operator+(const Matrix4x4& lhs, const Matrix4x4& rhs)
{
	Matrix4x4 tempMatrix = Matrix4x4(lhs);
	tempMatrix += rhs;
	return tempMatrix;
}

inline Matrix4x4 operator-(const Matrix4x4& lhs, const Matrix4x4& rhs)
{
	Matrix4x4 tempMatrix = Matrix4x4(lhs);
	tempMatrix -= rhs;
	return tempMatrix;
}


inline Matrix4x4 operator*(const Matrix4x4& lhs, const Matrix4x4& rhs)
{
	Matrix4x4 tempMatrix = Matrix4x4(lhs);
	tempMatrix *= rhs;
	return tempMatrix;
}

inline Matrix4x4 operator*(const Matrix4x4& lhs, const float& rhs)
{
	Matrix4x4 tempMatrix = Matrix4x4(lhs);
	tempMatrix *= rhs;
	return tempMatrix;
}

inline Vector4f operator*(const Matrix4x4 lhs, const Vector4f& rhs)
{
	// Since matrix is stored in row-major order:
	// Row 0: matrix[0], matrix[1], matrix[2], matrix[3]
	// Row 1: matrix[4], matrix[5], matrix[6], matrix[7]
	// Row 2: matrix[8], matrix[9], matrix[10], matrix[11]
	// Row 3: matrix[12], matrix[13], matrix[14], matrix[15]

	const float* m = lhs.getData();

	return Vector4f(
		m[0] * rhs.x + m[1] * rhs.y + m[2] * rhs.z + m[3] * rhs.w,    // Row 0 dot Vector
		m[4] * rhs.x + m[5] * rhs.y + m[6] * rhs.z + m[7] * rhs.w,    // Row 1 dot Vector
		m[8] * rhs.x + m[9] * rhs.y + m[10] * rhs.z + m[11] * rhs.w,  // Row 2 dot Vector
		m[12] * rhs.x + m[13] * rhs.y + m[14] * rhs.z + m[15] * rhs.w  // Row 3 dot Vector
	);
}

Matrix4x4 createTransformationMatrix(Vector3f translation, Vector3f rotation, Vector3f scale);
Matrix4x4 createViewMatrix(Vector3f position, Vector3f rotation);
Matrix4x4 createProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);
Matrix4x4 createOrthographicProjectionMatrix(float left, float right, float top, float bottom, float nearPlane, float farPlane);

float degreesToRadians(float degrees);