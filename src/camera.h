#pragma once

#include "commonUtils.h"
#include "matrix.h"

class Camera
{
public:
	Camera();
	Camera(Vector3f position, Vector3f rotation, float fov, float aspectRatio, float nearPlane, float farPlane);
	~Camera();

	Matrix4x4& getViewMatrix();
	Matrix4x4& getProjectionMatrix();

	Vector3f getForwardVector();
	Vector3f getRightVector();
	Vector3f getUpVector();

	void setPosition(Vector3f position);
	void setRotation(Vector3f rotation);
	void varyPosition(Vector3f deltaPosition);
	void varyRotation(Vector3f deltaRotation);

private:
	Vector3f position; // x, y, z
	Vector3f rotation; // pitch, yaw, roll
	Vector3f forward;
	Vector3f right;
	Vector3f up;

	Matrix4x4 projectionMatrix;
	Matrix4x4 viewMatrix;

	float fov;
	float aspectRatio;
	float nearPlane;
	float farPlane;

	void updateVectors();
};