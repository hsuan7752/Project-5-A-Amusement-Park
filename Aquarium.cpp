#include "Aquarium.H"
#define PI 3.1415926

Aquarium::
Aquarium()
{
	drawWater();
}

void Aquarium::
drawWater()
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0.0f, 0.0f, 1.0f);
	for (int i = 0; i <= 360; ++i)
		glVertex3f(radius * sinf(i * PI / 180), radius * cosf(i * PI / 180), 1.0f);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}