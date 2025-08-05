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
	float frustumLength = farPlane - nearPlane;

	projectionMatrix.getData()[0] = xScale;
	projectionMatrix.getData()[5] = yScale;
	projectionMatrix.getData()[10] = -(farPlane + nearPlane) / frustumLength;
	projectionMatrix.getData()[11] = -(2.0f * farPlane * nearPlane) / frustumLength;
	projectionMatrix.getData()[14] = -1.0f;
	projectionMatrix.getData()[15] = 0.0f;
	
	return projectionMatrix;
}

Matrix4x4 createOrthographicProjectionMatrix(float left, float right, float top, float bottom, float nearPlane, float farPlane)
{
	Matrix4x4 projectionMatrix;
	projectionMatrix.setToIdentity();

	float width = right - left;
	float height = top - bottom;
	float depth = farPlane - nearPlane;

	//ROW MAJOR ORDER
	projectionMatrix.getData()[0] = 2.0f / width; // [0][0]
	projectionMatrix.getData()[5] = 2.0f / height; // [1][1]
	projectionMatrix.getData()[10] = -2.0f / depth; // [2][2]
	projectionMatrix.getData()[12] = -(right + left) / width; // [0][3]
	projectionMatrix.getData()[13] = -(top + bottom) / height; // [1][3]
	projectionMatrix.getData()[14] = -(farPlane + nearPlane) / depth; // [2][3]
	projectionMatrix.getData()[15] = 1.0f; // [3][3]

	return projectionMatrix;
}

float degreesToRadians(float degrees)
{
	return ((degrees * PI) / 180.0f);
}