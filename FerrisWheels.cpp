#include "FerrisWheels.H"
#define PI 3.1415926

Carriage::
Carriage()
{
}

Carriage::
Carriage(glm::vec3 p)
	:position(p), radius(1.0f), slices(50.0f), stacks(50.0f)
{
	color = new float[3]{ 0, 100, 200 };
}

glm::vec3 Carriage::
getPosition()
{
	return position;
}

float* Carriage::
getColor()
{
	return color;
}

FerrisWheel::
FerrisWheel()
	:radius(4.0f)
{
	//carriages
	float c_radius = 5.0f;
	float c_angle = 0.0f;

	for (int i = 0; i < 12; ++i)
	{
		glm::vec2 pos = glm::vec2(0.0f, 0.0f);
		c_angle = i * 30;

		pos.x = c_radius * sinf(c_angle * PI / 180);
		pos.y = c_radius * cosf(c_angle * PI / 180);

		addCarriage(pos);
	}
}

void FerrisWheel::
addCarriage(glm::vec2 pos)
{
	Carriage newCarriage(glm::vec3(pos, 0.0f));

	carriages.push_back(newCarriage);
}

void FerrisWheel::
drawCarriage()
{
	GLUquadric* q = gluNewQuadric();
	gluSphere(q, 1, 50, 50);
}

void FerrisWheel::
draw(bool doingShadows)
{
	//draw carriages
	for (int i = 0; i < carriages.size(); ++i)
	{
		glPushMatrix();
		glTranslatef(carriages[i].getPosition().x, carriages[i].getPosition().y, carriages[i].getPosition().z);
		if (!doingShadows)
			glColor3fv(carriages[i].getColor());
		drawCarriage();
		glPopMatrix();
	}

	glBegin(GL_TRIANGLE_FAN);
	if (!doingShadows)
		glColor3ub(255, 255, 255);
	glVertex3f(0.0f, 0.0f, 1.0f);
	for (int i = 0; i <= 360; ++i)
		glVertex3f(radius * sinf(i * PI / 180), radius * cosf(i * PI / 180), 1.0f);
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	if (!doingShadows)
		glColor3ub(255, 255, 255);
	glVertex3f(0.0f, 0.0f, -1.0f);
	for (int i = 0; i <= 360; ++i)
		glVertex3f(radius * sinf(i * PI / 180), radius * cosf(i * PI / 180), -1.0f);
	glEnd();

	glBegin(GL_TRIANGLE_STRIP);
	if (!doingShadows)
		glColor3ub(255, 255, 255);
	for (int i = 0; i <= 360; ++i)
	{
		glVertex3f(radius * sinf(i * PI / 180.0), radius * cosf(i * PI / 180.0), 1.0f);
		glVertex3f(radius * sinf(i * PI / 180.0), radius * cosf(i * PI / 180.0), -1.0f);
	}
	glEnd();

	glBegin(GL_TRIANGLES);
	if (!doingShadows)
		glColor3ub(255, 255, 0);
	glVertex3f(0.0f, 0.0f, 1.0f);
	glVertex3f(-1.5f, -7.0f, 1.2f);
	glVertex3f(1.5f, -7.0f, 1.2f);
	glEnd();

	glBegin(GL_TRIANGLES);
	if (!doingShadows)
		glColor3ub(255, 255, 0);
	glVertex3f(0.0f, 0.0f, -1.0f);
	glVertex3f(-1.5f, -7.0f, -1.2f);
	glVertex3f(1.5f, -7.0f, -1.2f);
	glEnd();
}