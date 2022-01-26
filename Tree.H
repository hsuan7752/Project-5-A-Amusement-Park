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

#include "Utilities/3DUtils.H"

class Tree
{
public:
	Tree();

	void	draw(glm::vec3);

	void	drawCone(float radius);

	void	drawTrunk();
};