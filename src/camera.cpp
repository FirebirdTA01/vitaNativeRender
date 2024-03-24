#include "camera.h"


Camera::Camera()
{
	this->position = Vector3f(0.0f, 0.0f, 0.0f);
	this->rotation = Vector3f(0.0f, 0.0f, 0.0f);
	this->fov = 45.0f;
	this->aspectRatio = 16.0f / 9.0f;
	this->nearPlane = 0.1f;
	this->farPlane = 100.0f;

	this->viewMatrix = createViewMatrix(position, rotation);
	this->projectionMatrix = createProjectionMatrix(fov, aspectRatio, nearPlane, farPlane);

	this->updateVectors();
}

Camera::Camera(Vector3f pos, Vector3f rot, float fieldOfView, float aRatio, float nearP, float farP)
{
	this->position = pos;
	this->rotation = rot;
	this->fov = fieldOfView;
	this->aspectRatio = aRatio;
	this->nearPlane = nearP;
	this->farPlane = farP;

	this->viewMatrix = createViewMatrix(position, rotation);
	this->projectionMatrix = createProjectionMatrix(fov, aspectRatio, nearPlane, farPlane);

	this->updateVectors();
}

Camera::~Camera()
{

}

Matrix4x4& Camera::getProjectionMatrix()
{
	return projectionMatrix;
}

Matrix4x4& Camera::getViewMatrix()
{
	return viewMatrix;
}

Vector3f Camera::getForwardVector()
{
	return this->forward;
}

Vector3f Camera::getRightVector()
{
	return this->right;
}

Vector3f Camera::getUpVector()
{
	return this->up;
}

void Camera::setPosition(Vector3f pos)
{
	this->position = pos;
	this->viewMatrix = createViewMatrix(position, rotation);
}

void Camera::setRotation(Vector3f rot)
{
	this->rotation = rot;
	this->viewMatrix = createViewMatrix(position, rotation);
}

void Camera::varyPosition(Vector3f deltaPosition)
{
	this->position.x += deltaPosition.x;
	this->position.y += deltaPosition.y;
	this->position.z += deltaPosition.z;

	this->viewMatrix = createViewMatrix(position, rotation);
}

void Camera::varyRotation(Vector3f deltaRotation)
{
	this->rotation.x += deltaRotation.x;
	this->rotation.y += deltaRotation.y;
	this->rotation.z += deltaRotation.z;

	this->viewMatrix = createViewMatrix(position, rotation);
	this->updateVectors();
}

void Camera::updateVectors()
{
	//Angles are in degrees
	//Rotation.x is pitch, y is yaw, z is roll

	float* matrixData = viewMatrix.getData();

	this->forward = Vector3f(-matrixData[8], -matrixData[9], -matrixData[10]);
	this->right = Vector3f(matrixData[0], matrixData[1], matrixData[2]);
	this->up = Vector3f(matrixData[4], matrixData[5], matrixData[6]);
}