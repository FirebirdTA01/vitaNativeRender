#pragma once

#include "commonUtils.h"
#include <array>

//typedef float matrix4x4_multi[4][4];

typedef struct Matrix4x4
{
	Matrix4x4();
	Matrix4x4(std::array<float, 16> inMatrix);

	float* getData();
	//matrix4x4_multi* getDataMultidimensional();

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
	//matrix4x4_multi matrixMultidimensional;
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
		matrix[0] = tempMatrix[0] * rhs.matrix[0] + tempMatrix[1] * rhs.matrix[4] + tempMatrix[2] * rhs.matrix[8] + tempMatrix[3] * rhs.matrix[12];
		matrix[1] = tempMatrix[0] * rhs.matrix[1] + tempMatrix[1] * rhs.matrix[5] + tempMatrix[2] * rhs.matrix[9] + tempMatrix[3] * rhs.matrix[13];
		matrix[2] = tempMatrix[0] * rhs.matrix[2] + tempMatrix[1] * rhs.matrix[6] + tempMatrix[2] * rhs.matrix[10] + tempMatrix[3] * rhs.matrix[14];
		matrix[3] = tempMatrix[0] * rhs.matrix[3] + tempMatrix[1] * rhs.matrix[7] + tempMatrix[2] * rhs.matrix[11] + tempMatrix[3] * rhs.matrix[15];

		matrix[4] = tempMatrix[4] * rhs.matrix[0] + tempMatrix[5] * rhs.matrix[4] + tempMatrix[6] * rhs.matrix[8] + tempMatrix[7] * rhs.matrix[12];
		matrix[5] = tempMatrix[4] * rhs.matrix[1] + tempMatrix[5] * rhs.matrix[5] + tempMatrix[6] * rhs.matrix[9] + tempMatrix[7] * rhs.matrix[13];
		matrix[6] = tempMatrix[4] * rhs.matrix[2] + tempMatrix[5] * rhs.matrix[6] + tempMatrix[6] * rhs.matrix[10] + tempMatrix[7] * rhs.matrix[14];
		matrix[7] = tempMatrix[4] * rhs.matrix[3] + tempMatrix[5] * rhs.matrix[7] + tempMatrix[6] * rhs.matrix[11] + tempMatrix[7] * rhs.matrix[15];

		matrix[8] = tempMatrix[8] * rhs.matrix[0] + tempMatrix[9] * rhs.matrix[4] + tempMatrix[10] * rhs.matrix[8] + tempMatrix[11] * rhs.matrix[12];
		matrix[9] = tempMatrix[8] * rhs.matrix[1] + tempMatrix[9] * rhs.matrix[5] + tempMatrix[10] * rhs.matrix[9] + tempMatrix[11] * rhs.matrix[13];
		matrix[10] = tempMatrix[8] * rhs.matrix[2] + tempMatrix[9] * rhs.matrix[6] + tempMatrix[10] * rhs.matrix[10] + tempMatrix[11] * rhs.matrix[14];
		matrix[11] = tempMatrix[8] * rhs.matrix[3] + tempMatrix[9] * rhs.matrix[7] + tempMatrix[10] * rhs.matrix[11] + tempMatrix[11] * rhs.matrix[15];

		matrix[12] = tempMatrix[12] * rhs.matrix[0] + tempMatrix[13] * rhs.matrix[4] + tempMatrix[14] * rhs.matrix[8] + tempMatrix[15] * rhs.matrix[12];
		matrix[13] = tempMatrix[12] * rhs.matrix[1] + tempMatrix[13] * rhs.matrix[5] + tempMatrix[14] * rhs.matrix[9] + tempMatrix[15] * rhs.matrix[13];
		matrix[14] = tempMatrix[12] * rhs.matrix[2] + tempMatrix[13] * rhs.matrix[6] + tempMatrix[14] * rhs.matrix[10] + tempMatrix[15] * rhs.matrix[14];
		matrix[15] = tempMatrix[12] * rhs.matrix[3] + tempMatrix[13] * rhs.matrix[7] + tempMatrix[14] * rhs.matrix[11] + tempMatrix[15] * rhs.matrix[15];

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

float degreesToRadians(float degrees);