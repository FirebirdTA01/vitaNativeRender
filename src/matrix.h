#pragma once

#include "commonUtils.h"
#include <array>


typedef struct Matrix4x4
{
	Matrix4x4();
	Matrix4x4(std::array<float, 16> inMatrix);

	float* getData();

	//Sets the matrix to the identity matrix. THIS WILL OVERWRITE ANY DATA IN THE MATRIX
	void setToIdentity();

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
	float matrix[16];
public:


	//operators
	inline Matrix4x4& operator=(const Matrix4x4& rhs)
	{
		for (int i = 0; i < 16; i++)
		{
			matrix[i] = rhs.matrix[i];
		}
		return *this;
	}
	inline Matrix4x4& operator*=(const Matrix4x4& rhs)
	{
		float tempMatrix[16];
		for (int i = 0; i < 16; i++)
		{
			tempMatrix[i] = matrix[i];
		}

		for (int row = 0; row < 4; ++row)
		{
			for (int col = 0; col < 4; ++col)
			{
				matrix[row * 4 + col] =
					tempMatrix[row * 4 + 0] * rhs.matrix[0 * 4 + col] +
					tempMatrix[row * 4 + 1] * rhs.matrix[1 * 4 + col] +
					tempMatrix[row * 4 + 2] * rhs.matrix[2 * 4 + col] +
					tempMatrix[row * 4 + 3] * rhs.matrix[3 * 4 + col];
			}
		}

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

Matrix4x4 createTransformationMatrix(Vector3f translation, Vector3f rotation, Vector3f scale);
Matrix4x4 createViewMatrix(Vector3f position, Vector3f rotation);
Matrix4x4 createProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);
Matrix4x4 createOrthographicProjectionMatrix(float left, float right, float top, float bottom, float nearPlane, float farPlane);

float degreesToRadians(float degrees);