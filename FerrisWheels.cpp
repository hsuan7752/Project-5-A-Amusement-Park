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
	color = new float[3]{ 255, 255, 255 };
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
draw(bool doingShadows, double time)
{
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 1.8f);
	glRotatef(time * 360, 0.0f, 0.0f, 1.0f);
	drawWheel(doingShadows);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -1.8f);
	glRotatef(-time * 360, 0.0f, 0.0f, 1.0f);
	drawWheel(doingShadows);
	glPopMatrix();

	glBegin(GL_TRIANGLES);
	if (!doingShadows)
		glColor3ub(255, 255, 255);
	glVertex3f(0.0f, -2.0f, 2.8f);
	glVertex3f(-5.0f, -7.0f, 4.8f);
	glVertex3f(-3.0f, -7.0f, 4.8f);
	glEnd();

	glBegin(GL_TRIANGLES);
	if (!doingShadows)
		glColor3ub(255, 255, 255);
	glVertex3f(0.0f, -2.0f, 2.8f);
	glVertex3f(5.0f, -7.0f, 4.8f);
	glVertex3f(3.0f, -7.0f, 4.8f);
	glEnd();

	glBegin(GL_TRIANGLES);
	if (!doingShadows)
		glColor3ub(255, 255, 255);
	glVertex3f(0.0f, -2.0f, -2.8f);
	glVertex3f(-5.0f, -7.0f, -4.8f);
	glVertex3f(-3.0f, -7.0f, -4.8f);
	glEnd();

	glBegin(GL_TRIANGLES);
	if (!doingShadows)
		glColor3ub(255, 255, 255);
	glVertex3f(0.0f, -2.0f, -2.8f);
	glVertex3f(5.0f, -7.0f, -4.8f);
	glVertex3f(3.0f, -7.0f, -4.8f);
	glEnd();

	
}

void FerrisWheel::
drawWheel(bool doingShadows)
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
		glColor3ub(0, 0, 0);
	glVertex3f(0.0f, 0.0f, 1.0f);
	for (int i = 0; i <= 360; ++i)
		glVertex3f(radius * sinf(i * PI / 180), radius * cosf(i * PI / 180), 1.0f);
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	if (!doingShadows)
		glColor3ub(0, 0, 0);
	glVertex3f(0.0f, 0.0f, -1.0f);
	for (int i = 0; i <= 360; ++i)
		glVertex3f(radius * sinf(i * PI / 180), radius * cosf(i * PI / 180), -1.0f);
	glEnd();

	glBegin(GL_TRIANGLE_STRIP);
	if (!doingShadows)
		glColor3ub(0, 0, 0);
	for (int i = 0; i <= 360; ++i)
	{
		glVertex3f(radius * sinf(i * PI / 180.0), radius * cosf(i * PI / 180.0), 1.0f);
		glVertex3f(radius * sinf(i * PI / 180.0), radius * cosf(i * PI / 180.0), -1.0f);
	}
	glEnd();

	//draw Torus
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 1.0f);
	glScalef(12.0f, 12.0f, 12.0f);
	if (!doingShadows)
		glColor3ub(0, 0, 0);
	drawTorus(0.05, 0.50, 32, 32);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -1.0f);
	glScalef(12.0f, 12.0f, 12.0f);
	if (!doingShadows)
		glColor3ub(0, 0, 0);
	drawTorus(0.05, 0.50, 360, 360);
	glPopMatrix();
}

void FerrisWheel::
drawTorus(GLdouble innerRadius, GLdouble outerRadius, GLint nsides, GLint rings)
{
	int i, j;
	GLfloat theta, phi, theta1;
	GLfloat cosTheta, sinTheta;
	GLfloat cosTheta1, sinTheta1;
	GLfloat ringDelta, sideDelta;

	ringDelta = 2.0 * PI / rings;
	sideDelta = 2.0 * PI / nsides;

	theta = 0.0;
	cosTheta = 1.0;
	sinTheta = 0.0;
	for (i = rings - 1; i >= 0; i--) {
		theta1 = theta + ringDelta;
		cosTheta1 = cos(theta1);
		sinTheta1 = sin(theta1);
		glBegin(GL_QUAD_STRIP);
		phi = 0.0;
		for (j = nsides; j >= 0; j--) {
			GLfloat cosPhi, sinPhi, dist;

			phi += sideDelta;
			cosPhi = cos(phi);
			sinPhi = sin(phi);
			dist = outerRadius + innerRadius * cosPhi;

			glNormal3f(cosTheta1 * cosPhi, -sinTheta1 * cosPhi, sinPhi);
			glVertex3f(cosTheta1 * dist, -sinTheta1 * dist, innerRadius * sinPhi);
			glNormal3f(cosTheta * cosPhi, -sinTheta * cosPhi, sinPhi);
			glVertex3f(cosTheta * dist, -sinTheta * dist, innerRadius * sinPhi);
		}
		glEnd();
		theta = theta1;
		cosTheta = cosTheta1;
		sinTheta = sinTheta1;
	}
}