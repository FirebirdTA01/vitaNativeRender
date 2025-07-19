#include "commonUtils.h"

class Light
{
public:
	Light();
	Light(Vector3f pos, Color col);
	~Light();

	void setColor(Color color);
	void setPosition(Vector3f pos);
	void setRadius(float radius);
	void setPower(float power);


	Color getColor();
	Vector3f getPosition();
	float getRadius();
	float getPower();

private:
	Color color;
	Vector3f position;

	//max distance the light will travel
	float radius;

	//luminous power in lumens
	//used in combination with the radius to calculate the attenuation
	float power;
};