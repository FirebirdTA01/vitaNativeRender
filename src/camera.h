#pragma once

#include "commonUtils.h"
#include "matrix.h"

enum CameraType
{
	ORTHOGRAPHIC,
	PERSPECTIVE //default
};

class Camera
{
public:
	Camera();
	Camera(CameraType type, Vector3f position, Vector3f rotation, float fov, float aspectRatio, float nearPlane, float farPlane);
	~Camera();

	Matrix4x4& getViewMatrix();
	Matrix4x4& getProjectionMatrix();

	Vector3f getForwardVector() const;
	Vector3f getRightVector() const;
	Vector3f getUpVector() const;
	Vector3f getPosition() const;

	void setPosition(Vector3f position);
	void setRotation(Vector3f rotation);
	void varyPosition(Vector3f deltaPosition);
	void varyRotation(Vector3f deltaRotation);

	void recalculateProjectionMatrix();

private:
	CameraType type;
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