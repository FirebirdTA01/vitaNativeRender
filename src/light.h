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


	Color getColor() const;
	Vector3f getPosition() const;
	float getRadius() const;
	float getPower() const;

private:
	Color color;
	Vector3f position;

	//max distance the light will travel
	float radius;

	//luminous power in lumens
	//used in combination with the radius to calculate the attenuation
	float power;
};