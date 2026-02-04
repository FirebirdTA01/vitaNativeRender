#include "light.h"

Light::Light()
{
	color = Color();
	position = Vector3f();
	radius = 60.0f;
	power = 2400.0f;
}

Light::Light(Vector3f pos, Color col)
{
	color = col;
	position = pos;
	radius = 25.0f;
	power = 1200.0f;
}

Light::~Light()
{
}

void Light::setColor(Color col)
{
	color = col;
}

void Light::setPosition(Vector3f pos)
{
	position = pos;
}

void Light::setRadius(float rad)
{
	radius = rad;
}

void Light::setPower(float pow)
{
	power = pow;
}

Color Light::getColor() const
{
	return this->color;
}

Vector3f Light::getPosition() const
{
	return this->position;
}

float Light::getRadius() const
{
	return this->radius;
}

float Light::getPower() const
{
	return this->power;
}