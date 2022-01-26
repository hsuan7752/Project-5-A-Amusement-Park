#pragma once

#include <iostream>
#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "GL/glu.h"
#include <math.h>
#include <vector>
#include <glm/glm.hpp>
//#include <GLUT/glut.h>

#include "Utilities/3DUtils.H"

class Carriage
{
public:
	Carriage();
	Carriage(glm::vec3 p);
	glm::vec3			getPosition();
	float*			getColor();

private:
	float				radius = 0.0f;
	float				slices = 0.0f;
	float				stacks = 0.0f;

	glm::vec3			position;
	float*				color;
};

class FerrisWheel
{
public:
	FerrisWheel();

	void addCarriage(glm::vec2 pos);

	void drawCarriage();

	void draw(bool doingShadows, double time);

	void drawWheel(bool doingShadows);

	void drawTorus(GLdouble innerRadius, GLdouble outerRadius,
		GLint nsides, GLint rings);

private:
	std::vector< Carriage >		carriages;
	float						radius;
};
