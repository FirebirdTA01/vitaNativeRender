#include "matrix.h"
#include "commonUtils.h"

#include <cmath>


Matrix4x4::Matrix4x4()
{
	//for (int i = 0; i < 16; i++)
	//{
	std::fill(matrix, matrix + sizeof(matrix) / sizeof(float), 0.0f);
	//}
}

Matrix4x4::Matrix4x4(std::array<float, 16> inMatrix)
{
	for (int i = 0; i < 16; i++)
	{
		matrix[i] = inMatrix[i];
	}
}

float* Matrix4x4::getData()
{
	return matrix;
}

/*
matrix4x4_multi* Matrix4x4::getDataMultidimensional()
{
	matrixMultidimensional[0][0] = matrix[0];
	matrixMultidimensional[0][1] = matrix[1];
	matrixMultidimensional[0][2] = matrix[2];
	matrixMultidimensional[0][3] = matrix[3];

	matrixMultidimensional[1][0] = matrix[4];
	matrixMultidimensional[1][1] = matrix[5];
	matrixMultidimensional[1][2] = matrix[6];
	matrixMultidimensional[1][3] = matrix[7];

	matrixMultidimensional[2][0] = matrix[8];
	matrixMultidimensional[2][1] = matrix[9];
	matrixMultidimensional[2][2] = matrix[10];
	matrixMultidimensional[2][3] = matrix[11];

	matrixMultidimensional[3][0] = matrix[12];
	matrixMultidimensional[3][1] = matrix[13];
	matrixMultidimensional[3][2] = matrix[14];
	matrixMultidimensional[3][3] = matrix[15];

	return &matrixMultidimensional;
}*/

void Matrix4x4::setToIdentity()
{
	std::fill(matrix, matrix + sizeof(matrix) / sizeof(float), 0.0f);

	//set diagnal elements because identity matrix is intended to work with 3D space
	//ROW MAJOR ORDER
	matrix[0] = 1.0f;
	matrix[5] = 1.0f;
	matrix[10] = 1.0f;
	matrix[15] = 1.0f;
}

void Matrix4x4::translate(Vector3f translation)
{
	std::array<float, 16> transformElements =
	{
		1.0f, 0.0f, 0.0f, translation.x,
		0.0f, 1.0f, 0.0f, translation.y,
		0.0f, 0.0f, 1.0f, translation.z,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	Matrix4x4 transform = Matrix4x4(transformElements);

	*this *= transform;
}

void Matrix4x4::rotateX(float angle)
{
	std::array<float, 16> transformElements =
	{
		1.0f,		0.0f,				0.0f,			0.0f,
		0.0f,	cosf(-angle),		-sinf(-angle),		0.0f,
		0.0f,	sinf(-angle),		cosf(-angle),		0.0f,
		0.0f,		0.0f,				0.0f,			1.0f
	};
	Matrix4x4 transform = Matrix4x4(transformElements);

	*this *= transform;
}

void Matrix4x4::rotateY(float angle)
{
	std::array<float, 16> transformElements =
	{
		cosf(-angle),		0.0f,	sinf(-angle),		0.0f,
		0.0f,				1.0f,		0.0f,			0.0f,
		-sinf(-angle),		0.0f,	cosf(-angle),		0.0f,
		0.0f,				0.0f,		0.0f,			1.0f
	};
	Matrix4x4 transform = Matrix4x4(transformElements);

	*this *= transform;
}

void Matrix4x4::rotateZ(float angle)
{
	std::array<float, 16> transformElements =
	{
		cosf(-angle),		-sinf(-angle),		0.0f,	0.0f,
		sinf(-angle),		cosf(-angle),		0.0f,	0.0f,
		0.0f,					0.0f,			1.0f,	0.0f,
		0.0f,					0.0f,			0.0f,	1.0f
	};
	Matrix4x4 transform = Matrix4x4(transformElements);

	*this *= transform;
}

void Matrix4x4::rotate(Vector3f angles)
{
	rotateX(angles.x);
	rotateY(angles.y);
	rotateZ(angles.z);
}

void Matrix4x4::scale(Vector3f scale)
{
	std::array<float, 16> transformElements =
	{
		scale.x,	0.0f,		0.0f,		0.0f,
		0.0f,		scale.y,	0.0f,		0.0f,
		0.0f,		0.0f,		scale.z,	0.0f,
		0.0f,		0.0f,		0.0f,		1.0f
	};
	Matrix4x4 transform = Matrix4x4(transformElements);

	*this *= transform;
}

void Matrix4x4::scale(float scale)
{
	std::array<float, 16> transformElements =
	{
		scale,		0.0f,		0.0f,		0.0f,
		0.0f,		scale,		0.0f,		0.0f,
		0.0f,		0.0f,		scale,		0.0f,
		0.0f,		0.0f,		0.0f,		1.0f
	};
	Matrix4x4 transform = Matrix4x4(transformElements);

	*this *= transform;
}

Matrix4x4 createTransformationMatrix(Vector3f translation, Vector3f rotation, Vector3f scale)
{
	Matrix4x4 transformationMatrix;
	transformationMatrix.setToIdentity();

	transformationMatrix.translate(translation);
	transformationMatrix.rotate(rotation);
	transformationMatrix.scale(scale);

	return transformationMatrix;
}


Matrix4x4 createViewMatrix(Vector3f position, Vector3f rotation)
{
	Matrix4x4 viewMatrix;
	viewMatrix.setToIdentity();

	viewMatrix.rotate(rotation);
	viewMatrix.translate(position * -1);

	return viewMatrix;
}

Matrix4x4 createProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	Matrix4x4 projectionMatrix;
	projectionMatrix.setToIdentity();

	float yScale = (float)(1.0f / tanf(degreesToRadians(fov) / 2.0f));
	float xScale = yScale / aspectRatio;
	float frustumLength = nearPlane - farPlane;

	projectionMatrix.getData()[0] = xScale;
	projectionMatrix.getData()[5] = yScale;
	projectionMatrix.getData()[10] = (farPlane + nearPlane) / frustumLength;
	projectionMatrix.getData()[11] = (2.0f * farPlane * nearPlane) / frustumLength;
	projectionMatrix.getData()[14] = -1.0f;

	return projectionMatrix;
}

float degreesToRadians(float degrees)
{
	return ((degrees * PI) / 180.0f);
}