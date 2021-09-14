#include "Train.H"
#include "glad/glad.h"
#include <math.h>
#include <iostream>
#include "TrainView.H"
#include "TrainWindow.H"

Train::
Train(Pnt3f pos, Pnt3f orient, float* vec, bool doingShadow)
{
	this->pos = pos;

	this->orient = orient;

	Pnt3f AC(vec[0], vec[1], vec[2]);
	Pnt3f AB(-9.0f, 0.0f, 0.0f);

	float cos_y = (float)(AC.x * AB.x + AC.y * AB.y + AC.z * AB.z)
		/ (abs(sqrt(pow(AC.x, 2) + pow(AC.y, 2) + pow(AC.z, 2))) * abs(sqrt(pow(AB.x, 2) + pow(AB.y, 2) + pow(AB.z, 2))));
	float sin_y = (float)sqrt(1 - pow(cos_y, 2));

	if ((AC.x < 0 && AC.z > 0) || (AC.x > 0 && AC.z > 0))
				sin_y = -sin_y;

	float rotationMatrix[3][3] = { { cos_y, 0, -sin_y },
										  { 0, 1, 0 },
										  { sin_y, 0, cos_y } };
	float* v0 = new float[3]{ -9, 7, +3 };
	float* v1 = new float[3]{ 5, 7, +3 };
	float* v2 = new float[3]{ 5, 7, -3 };
	float* v3 = new float[3]{ -9, 7, -3 };
	
	float* v4 = new float[3]{ -9, 1, +3 };
	float* v5 = new float[3]{ 5, 1, +3 };
	float* v6 = new float[3]{ 5, 1, -3 };
	float* v7 = new float[3]{ -9, 1, -3 };

	v0 = rotatef(rotationMatrix, v0);
	v1 = rotatef(rotationMatrix, v1);
	v2 = rotatef(rotationMatrix, v2);
	v3 = rotatef(rotationMatrix, v3);

	v4 = rotatef(rotationMatrix, v4);
	v5 = rotatef(rotationMatrix, v5);
	v6 = rotatef(rotationMatrix, v6);
	v7 = rotatef(rotationMatrix, v7);

	////left
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(240, 100, 100);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + v7[0], pos.y + v7[1], pos.z + v7[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + v6[0], pos.y + v6[1], pos.z + v6[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + v2[0], pos.y + v2[1], pos.z + v2[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + v3[0], pos.y + v3[1], pos.z + v3[2]);
	//glEnd();

	////right
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(240, 100, 100);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + v4[0], pos.y + v4[1], pos.z + v4[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + v5[0], pos.y + v5[1], pos.z + v5[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + v1[0], pos.y + v1[1], pos.z + v1[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + v0[0], pos.y + v0[1], pos.z + v0[2]);
	//glEnd();

	////front
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	//glColor3ub(120, 100, 200);
	//	glColor3ub(240, 100, 100);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + v7[0], pos.y + v7[1], pos.z + v7[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + v4[0], pos.y + v4[1], pos.z + v4[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + v0[0], pos.y + v0[1], pos.z + v0[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + v3[0], pos.y + v3[1], pos.z + v3[2]);
	//glEnd();

	////back
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(240, 100, 100);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + v6[0], pos.y + v6[1], pos.z + v6[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + v5[0], pos.y + v5[1], pos.z + v5[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + v1[0], pos.y + v1[1], pos.z + v1[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + v2[0], pos.y + v2[1], pos.z + v2[2]);
	//glEnd();

	////up
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(240, 100, 100);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + v0[0], pos.y + v0[1], pos.z + v0[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + v1[0], pos.y + v1[1], pos.z + v1[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + v2[0], pos.y + v2[1], pos.z + v2[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + v3[0], pos.y + v3[1], pos.z + v3[2]);
	//glEnd();

	////down
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(240, 100, 100);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + v4[0], pos.y + v4[1], pos.z + v4[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + v5[0], pos.y + v5[1], pos.z + v5[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + v6[0], pos.y + v6[1], pos.z + v6[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + v7[0], pos.y + v7[1], pos.z + v7[2]);
	//glEnd();

	float* f_c0 = new float[3]{ -2, 7, +3 };
	float* f_c1 = new float[3]{ -2, 7, -3 };
	float* f_c2 = new float[3]{ -2, 10, -3 };
	float* f_c3 = new float[3]{ -2, 10, +3 };

	float* f_c4 = new float[3]{ 4, 7, +3 };
	float* f_c5 = new float[3]{ 4, 7, -3 };
	float* f_c6 = new float[3]{ 4, 10, -3 };
	float* f_c7 = new float[3]{ 4, 10, +3 };

	f_c0 = rotatef(rotationMatrix, f_c0);
	f_c1 = rotatef(rotationMatrix, f_c1);
	f_c2 = rotatef(rotationMatrix, f_c2);
	f_c3 = rotatef(rotationMatrix, f_c3);

	f_c4 = rotatef(rotationMatrix, f_c4);
	f_c5 = rotatef(rotationMatrix, f_c5);
	f_c6 = rotatef(rotationMatrix, f_c6);
	f_c7 = rotatef(rotationMatrix, f_c7);

	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	glBegin(GL_QUADS);	
	if (!doingShadow)
		glColor3ub(255, 255, 255);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-5, 7, 3);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(5, 7, 3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(5, 7, -3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-5, 7, -3);
	glEnd();
	glPopMatrix();

	////front
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(200, 180, 150);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + f_c0[0], pos.y + f_c0[1], pos.z + f_c0[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + f_c1[0], pos.y + f_c1[1], pos.z + f_c1[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + f_c2[0], pos.y + f_c2[1], pos.z + f_c2[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + f_c3[0], pos.y + f_c3[1], pos.z + f_c3[2]);
	//glEnd();

	////back
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(200, 180, 150);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + f_c4[0], pos.y + f_c4[1], pos.z + f_c4[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + f_c5[0], pos.y + f_c5[1], pos.z + f_c5[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + f_c6[0], pos.y + f_c6[1], pos.z + f_c6[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + f_c7[0], pos.y + f_c7[1], pos.z + f_c7[2]);
	//glEnd();

	////left
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(200, 180, 150);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + f_c4[0], pos.y + f_c4[1], pos.z + f_c4[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + f_c0[0], pos.y + f_c0[1], pos.z + f_c0[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + f_c3[0], pos.y + f_c3[1], pos.z + f_c3[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + f_c7[0], pos.y + f_c7[1], pos.z + f_c7[2]);

	////right
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(200, 180, 150);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + f_c5[0], pos.y + f_c5[1], pos.z + f_c5[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + f_c1[0], pos.y + f_c1[1], pos.z + f_c1[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + f_c2[0], pos.y + f_c2[1], pos.z + f_c2[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + f_c6[0], pos.y + f_c6[1], pos.z + f_c6[2]);

	////up
	//float* h0 = new float[3]{ -3, 10, +3 };
	//float* h1 = new float[3]{ -3, 10, -3 };
	//float* h2 = new float[3]{ -3, 12, -3 };
	//float* h3 = new float[3]{ -3, 12, +3 };
	//	   
	//float* h4 = new float[3]{ 5, 10, +3 };
	//float* h5 = new float[3]{ 5, 10, -3 };
	//float* h6 = new float[3]{ 5, 12, -3 };
	//float* h7 = new float[3]{ 5, 12, +3 };

	//h0 = rotatef(rotationMatrix, h0);
	//h1 = rotatef(rotationMatrix, h1);
	//h2 = rotatef(rotationMatrix, h2);
	//h3 = rotatef(rotationMatrix, h3);
	//							 
	//h4 = rotatef(rotationMatrix, h4);
	//h5 = rotatef(rotationMatrix, h5);
	//h6 = rotatef(rotationMatrix, h6);
	//h7 = rotatef(rotationMatrix, h7);

	////front
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(150, 120, 0);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + h0[0], pos.y + h0[1], pos.z + h0[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + h1[0], pos.y + h1[1], pos.z + h1[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + h2[0], pos.y + h2[1], pos.z + h2[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + h3[0], pos.y + h3[1], pos.z + h3[2]);
	//glEnd();

	////back
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(200, 0, 0);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + h4[0], pos.y + h4[1], pos.z + h4[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + h5[0], pos.y + h5[1], pos.z + h5[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + h6[0], pos.y + h6[1], pos.z + h6[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + h7[0], pos.y + h7[1], pos.z + h7[2]);
	//glEnd();

	////left
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(200, 0, 0);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + h4[0], pos.y + h4[1], pos.z + h4[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + h0[0], pos.y + h0[1], pos.z + h0[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + h3[0], pos.y + h3[1], pos.z + h3[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + h7[0], pos.y + h7[1], pos.z + h7[2]);

	////right
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(200, 0, 0);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + h5[0], pos.y + h5[1], pos.z + h5[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + h1[0], pos.y + h1[1], pos.z + h1[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + h2[0], pos.y + h2[1], pos.z + h2[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + h6[0], pos.y + h6[1], pos.z + h6[2]);

	////up
	//glBegin(GL_QUADS);
	//if (!doingShadow)
	//	glColor3ub(200, 0, 0);
	//glTexCoord2f(0.0f, 0.0f);
	//glVertex3f(pos.x + h7[0], pos.y + h7[1], pos.z + h7[2]);
	//glTexCoord2f(0.0f, 1.0f);
	//glVertex3f(pos.x + h3[0], pos.y + h3[1], pos.z + h3[2]);
	//glTexCoord2f(1.0f, 1.0f);
	//glVertex3f(pos.x + h2[0], pos.y + h2[1], pos.z + h2[2]);
	//glTexCoord2f(1.0f, 0.0f);
	//glVertex3f(pos.x + h6[0], pos.y + h6[1], pos.z + h6[2]);
}

Train::
Train(Pnt3f pos)
{
	this->pos = pos;
}

void Train::
add(Pnt3f pos, Pnt3f orient, float* vec, bool doingShadow)
{
	next_car = new Train(pos);
	next_car->draw(doingShadow, vec);
}

void Train::
del()
{
	
}

void Train::
draw(bool doingShadow, float* vec)
{
	Pnt3f AC(vec[0], vec[1], vec[2]);
	Pnt3f AB(-9.0f, 0.0f, 0.0f);

	float cos_y = (float)(AC.x * AB.x + AC.y * AB.y + AC.z * AB.z)
		/ (abs(sqrt(pow(AC.x, 2) + pow(AC.y, 2) + pow(AC.z, 2))) * abs(sqrt(pow(AB.x, 2) + pow(AB.y, 2) + pow(AB.z, 2))));
	float sin_y = (float)sqrt(1 - pow(cos_y, 2));

	if ((AC.x < 0 && AC.z > 0) || (AC.x > 0 && AC.z > 0))
		sin_y = -sin_y;

	float rotationMatrix[3][3] = { { cos_y, 0, -sin_y },
										  { 0, 1, 0 },
										  { sin_y, 0, cos_y } };
	float* v0 = new float[3]{ -5, 7, +3 };
	float* v1 = new float[3]{ 5, 7, +3 };
	float* v2 = new float[3]{ 5, 7, -3 };
	float* v3 = new float[3]{ -5, 7, -3 };

	float* v4 = new float[3]{ -5, 1, +3 };
	float* v5 = new float[3]{ 5, 1, +3 };
	float* v6 = new float[3]{ 5, 1, -3 };
	float* v7 = new float[3]{ -5, 1, -3 };

	v0 = rotatef(rotationMatrix, v0);
	v1 = rotatef(rotationMatrix, v1);
	v2 = rotatef(rotationMatrix, v2);
	v3 = rotatef(rotationMatrix, v3);

	v4 = rotatef(rotationMatrix, v4);
	v5 = rotatef(rotationMatrix, v5);
	v6 = rotatef(rotationMatrix, v6);
	v7 = rotatef(rotationMatrix, v7);

	//left
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(200, 100, 160);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x + v7[0], pos.y + v7[1], pos.z + v7[2]);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + v6[0], pos.y + v6[1], pos.z + v6[2]);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + v2[0], pos.y + v2[1], pos.z + v2[2]);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x + v3[0], pos.y + v3[1], pos.z + v3[2]);
	glEnd();

	//right
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(200, 100, 160);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x + v4[0], pos.y + v4[1], pos.z + v4[2]);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + v5[0], pos.y + v5[1], pos.z + v5[2]);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + v1[0], pos.y + v1[1], pos.z + v1[2]);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x + v0[0], pos.y + v0[1], pos.z + v0[2]);
	glEnd();

	//front
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(200, 100, 160);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x + v7[0], pos.y + v7[1], pos.z + v7[2]);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + v4[0], pos.y + v4[1], pos.z + v4[2]);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + v0[0], pos.y + v0[1], pos.z + v0[2]);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x + v3[0], pos.y + v3[1], pos.z + v3[2]);
	glEnd();

	//back
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(200, 100, 160);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x + v6[0], pos.y + v6[1], pos.z + v6[2]);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + v5[0], pos.y + v5[1], pos.z + v5[2]);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + v1[0], pos.y + v1[1], pos.z + v1[2]);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x + v2[0], pos.y + v2[1], pos.z + v2[2]);
	glEnd();

	//up
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(200, 100, 160);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x + v0[0], pos.y + v0[1], pos.z + v0[2]);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + v1[0], pos.y + v1[1], pos.z + v1[2]);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + v2[0], pos.y + v2[1], pos.z + v2[2]);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x + v3[0], pos.y + v3[1], pos.z + v3[2]);
	glEnd();

	//down
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(200, 100, 160);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x + v4[0], pos.y + v4[1], pos.z + v4[2]);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + v5[0], pos.y + v5[1], pos.z + v5[2]);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + v6[0], pos.y + v6[1], pos.z + v6[2]);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x + v7[0], pos.y + v7[1], pos.z + v7[2]);
	glEnd();
}

void Train::
firstCar(bool doingShadow)
{
	//left
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(120, 100, 200);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x - 9, pos.y + 1, pos.z - 3);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 1, pos.z - 3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 7, pos.z - 3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x - 9, pos.y + 7, pos.z - 3);
	glEnd();

	//right
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(120, 100, 200);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x - 9, pos.y + 1, pos.z + 3);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 1, pos.z + 3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 7, pos.z + 3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x - 9, pos.y + 7, pos.z + 3);
	glEnd();

	//front
	glBegin(GL_QUADS);
	if (!doingShadow)
		//glColor3ub(120, 100, 200);
		glColor3ub(255, 255, 255);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x - 9, pos.y + 1, pos.z - 3);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x - 9, pos.y + 1, pos.z + 3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x - 9, pos.y + 7, pos.z + 3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x - 9, pos.y + 7, pos.z - 3);
	glEnd();

	//back
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(120, 100, 200);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x + 9, pos.y + 1, pos.z - 3);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 1, pos.z + 3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 7, pos.z + 3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x + 9, pos.y + 7, pos.z - 3);
	glEnd();

	//up
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(120, 100, 200);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x - 9, pos.y + 7, pos.z + 3);
	glTexCoord2f(0.1f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 7, pos.z + 3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 7, pos.z - 3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x - 9, pos.y + 7, pos.z - 3);
	glEnd();

	//down
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(120, 100, 200);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x - 9, pos.y + 1, pos.z + 3);
	glTexCoord2f(0.1f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 1, pos.z + 3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 1, pos.z - 3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x - 9, pos.y + 1, pos.z - 3);
	glEnd();


	glBegin(GL_QUADS);
	if (!doingShadow)
		//glColor3ub(120, 100, 200);
		glColor3ub(255, 255, 255);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x + 3, pos.y + 6, pos.z + 4);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 6, pos.z + 4);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 12, pos.z + 4);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x + 3, pos.y + 12, pos.z + 4);
	glEnd();

	glBegin(GL_QUADS);
	if (!doingShadow)
		//glColor3ub(120, 100, 200);
		glColor3ub(255, 255, 255);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x + 3, pos.y + 6, pos.z - 4);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 6, pos.z - 4);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 12, pos.z - 4);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x + 3, pos.y + 12, pos.z - 4);
	glEnd();

	glBegin(GL_QUADS);
	if (!doingShadow)
		//glColor3ub(120, 100, 200);
		glColor3ub(255, 255, 255);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x + 3, pos.y + 6, pos.z + 4);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + 3, pos.y + 6, pos.z - 4);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + 3, pos.y + 12, pos.z - 4);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x + 3, pos.y + 12, pos.z + 4);
	glEnd();

	glBegin(GL_QUADS);
	if (!doingShadow)
		//glColor3ub(120, 100, 200);
		glColor3ub(255, 255, 255);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x + 9, pos.y + 6, pos.z + 4);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 6, pos.z - 4);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 12, pos.z - 4);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x + 9, pos.y + 12, pos.z + 4);
	glEnd();

	glBegin(GL_QUADS);
	if (!doingShadow)
		//glColor3ub(120, 100, 200);
		glColor3ub(255, 255, 255);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(pos.x + 9, pos.y + 12, pos.z + 4);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(pos.x + 9, pos.y + 12, pos.z - 4);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(pos.x + 3, pos.y + 12, pos.z - 4);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(pos.x + 3, pos.y + 12, pos.z + 4);
	glEnd();
}

float* Train::
rotatef(float m[][3], float* p)
{
	float* n = new float[] {0, 0, 0};
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			n[i] += m[i][j] * p[j];

	return n;
}