#include "Tree.H"
#define PI 3.1415926

Tree::Tree()
{
	
}

void Tree::
draw(glm::vec3 pos)
{
	glPushMatrix();

	glTranslatef(pos.x, 15.0f, pos.z);

	glPushMatrix();
	glTranslatef(0.0f, 2.0f, 0.0f);
	drawCone(5.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, -3.0f, 0.0f);
	drawCone(8.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, -8.0f, 0.0f);
	drawCone(10.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, -10.0f, 0.0f);
	glColor3ub(50, 50, 10);
	drawTrunk();
	glPopMatrix();

	glPopMatrix();
}

void Tree::
drawCone(float radius)
{
	glBegin(GL_TRIANGLE_FAN);
	//if (!doingShadows)
	glColor3ub(0, 80, 0);
	glVertex3f(0.0f, radius, 0.0f);
	for (int i = 0; i <= 360; ++i)
		glVertex3f(radius * sinf(i * PI / 180), 0.0f, radius * cosf(i * PI / 180));
	glEnd();
}

void Tree::
drawTrunk()
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float r = 2.0f;

	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(x, y + 1.0f, z);
	for (int i = 0; i <= 360; ++i)
		glVertex3f(x + r * cos(i * PI / 180.0), y + 1.0f, z + r * sin(i * PI / 180.0));
	glEnd();

	glBegin(GL_TRIANGLE_FAN);

	glVertex3f(x, y - 1.0f, z);
	for (int i = 0; i <= 360; ++i)
		glVertex3f(x + r * cos(i * PI / 180.0), y - 1.0f, z + r * sin(i * PI / 180.0));
	glEnd();

	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i <= 360; ++i)
	{
		glVertex3f(r * cos(i * PI / 180.0), 5.0f, r * sin(i * PI / 180.0));
		glVertex3f(r * cos(i * PI / 180.0), -5.0f, r * sin(i * PI / 180.0));
	}
	glEnd();
}