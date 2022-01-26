/************************************************************************
	 File:        TrainView.cpp
	 Author:
				  Michael Gleicher, gleicher@cs.wisc.edu
	 Modifier
				  Yu-Chi Lai, yu-chi@cs.wisc.edu

	 Comment:
						The TrainView is the window that actually shows the
						train. Its a
						GL display canvas (Fl_Gl_Window).  It is held within
						a TrainWindow
						that is the outer window with all the widgets.
						The TrainView needs
						to be aware of the window - since it might need to
						check the widgets to see how to draw
	  Note:        we need to have pointers to this, but maybe not know
						about it (beware circular references)
	 Platform:    Visio Studio.Net 2003/2005
*************************************************************************/

#include <iostream>
#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
//#include "GL/gl.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <math.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb-master/stb_image.h"
#include <iostream>
#include <algorithm>

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"
#include "Train.H"
#include "FerrisWheels.H"


#ifdef EXAMPLE_SOLUTION
#	include "TrainExample/TrainExample.H"
#endif

enum splineType
{
	LINEAR = 1, CARDINAL, B_SPLINE
};

enum trackType
{
	SIMPLE = 1, PARALLEL, ROAD
};


float M_cardinal[4][4]{ { -0.5,  1.5, -1.5,  0.5 },
									{    1, -2.5,    2, -0.5 },
									{ -0.5,    0,  0.5,    0 },
									{    0,    1,    0,    0 } };

float M_b_spline[4][4]{ { -0.1667,    0.5,   -0.5, 0.1667 },
						{     0.5,     -1,    0.5,      0 },
						{    -0.5,      0,    0.5,      0 },
						{  0.1667, 0.6667, 0.1667,      0 } };

//************************************************************************
//
// * Constructor to set up the GL window
//========================================================================
TrainView::
TrainView(int x, int y, int w, int h, const char* l)
	: Fl_Gl_Window(x, y, w, h, l)
	//========================================================================
{
	mode(FL_RGB | FL_ALPHA | FL_DOUBLE | FL_STENCIL);

	resetArcball();
}

//************************************************************************
//
// * Reset the camera to look at the world
//========================================================================
void TrainView::
resetArcball()
//========================================================================
{
	// Set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this, 40, 250, .2f, .4f, 0);
}

//************************************************************************
//
// * FlTk Event handler for the window
//########################################################################
// TODO: 
//       if you want to make the train respond to other events 
//       (like key presses), you might want to hack this.
//########################################################################
//========================================================================
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, 
	// then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event))
			return 1;

	// remember what button was used
	static int last_push;

	switch (event) {
		// Mouse button being pushed event
	case FL_PUSH:
		last_push = Fl::event_button();
		// if the left button be pushed is left mouse button
		if (last_push == FL_LEFT_MOUSE) {
			doPick();
			damage(1);
			return 1;
		};
		break;

		// Mouse button release event
	case FL_RELEASE: // button release
		damage(1);
		last_push = 0;
		return 1;

		// Mouse button drag event
	case FL_DRAG:

		// Compute the new control point position
		if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
			ControlPoint* cp = &m_pTrack->points[selectedCube];

			double r1x, r1y, r1z, r2x, r2y, r2z;
			getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

			double rx, ry, rz;
			mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z,
				static_cast<double>(cp->pos.x),
				static_cast<double>(cp->pos.y),
				static_cast<double>(cp->pos.z),
				rx, ry, rz,
				(Fl::event_state() & FL_CTRL) != 0);

			cp->pos.x = (float)rx;
			cp->pos.y = (float)ry;
			cp->pos.z = (float)rz;
			damage(1);
		}
		break;

		// in order to get keyboard events, we need to accept focus
	case FL_FOCUS:
		return 1;

		// every time the mouse enters this window, aggressively take focus
	case FL_ENTER:
		focus(this);
		break;

	case FL_KEYBOARD:
		int k = Fl::event_key();
		int ks = Fl::event_state();
		if (k == 'p') {
			// Print out the selected control point information
			if (selectedCube >= 0)
				printf("Selected(%d) (%g %g %g) (%g %g %g)\n",
					selectedCube,
					m_pTrack->points[selectedCube].pos.x,
					m_pTrack->points[selectedCube].pos.y,
					m_pTrack->points[selectedCube].pos.z,
					m_pTrack->points[selectedCube].orient.x,
					m_pTrack->points[selectedCube].orient.y,
					m_pTrack->points[selectedCube].orient.z);
			else
				printf("Nothing Selected\n");

			return 1;
		};
		break;
	}

	return Fl_Gl_Window::handle(event);
}

//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================
void TrainView::draw()
{

	//*********************************************************************
	//
	// * Set up basic opengl informaiton
	//
	//**********************************************************************
	//initialized glad
	if (gladLoadGL())
	{
		srand(time(NULL));
		//initiailize VAO, VBO, Shader...
		if (!this->skyboxShader)
			this->initskyboxShader();

		if (!this->planeShader)
			this->initPlaneShader();

		if (!this->heightMapShader)
			this->initHeightMapShader();

		if (!this->tilesShader)
			this->initTilesShader();

		//particles = new Particle();
		//InitParticle(*particles);
		nOfFires = 0;

		if (!this->commom_matrices)
			this->commom_matrices = new UBO();
		this->commom_matrices->size = 2 * sizeof(glm::mat4);
		glGenBuffers(1, &this->commom_matrices->ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
		glBufferData(GL_UNIFORM_BUFFER, this->commom_matrices->size, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");
	ProcessParticles();
	// Set up the view port
	glViewport(0, 0, w(), h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0, 0, .3f, 0);		// background should be blue

	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	//######################################################################
	// TODO: 
	// you might want to set the lighting up differently. if you do, 
	// we need to set up the lights AFTER setting up the projection
	//######################################################################
	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// top view only needs one light
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
		glDisable(GL_LIGHT3);
	}
	else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
		glEnable(GL_LIGHT3);
	}


	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************
	GLfloat lightPosition1[] = { 0,1,1,0 }; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition2[] = { 1, 0, 0, 0 };
	GLfloat lightPosition3[] = { 0, -1, 0, 0 };
	GLfloat yellowLight[] = { 0.5f, 0.5f, .1f, 1.0 };
	GLfloat whiteLight[] = { 1.0f, 1.0f, 1.0f, 1.0 };
	GLfloat blueLight[] = { .1f,.1f,.3f,1.0 };
	GLfloat grayLight[] = { .3f, .3f, .3f, 1.0 };

	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);
	
	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);
	
	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);

	// set linstener position
	if (selectedCube >= 0)
		alListener3f(AL_POSITION,
			m_pTrack->points[selectedCube].pos.x,
			m_pTrack->points[selectedCube].pos.y,
			m_pTrack->points[selectedCube].pos.z);
	else
		alListener3f(AL_POSITION,
			this->source_pos.x,
			this->source_pos.y,
			this->source_pos.z);

	glEnable(GL_LIGHTING);
	setupObjects();

	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	glDisable(GL_LIGHTING);

	//*********************************************************************
	// now draw the object and we need to do it twice
	// once for real, and then once for shadows
	//*********************************************************************
	glEnable(GL_LIGHTING);
	setupObjects();

	drawStuff();

	// this time drawing is for shadows (except for top view)
	if (!tw->topCam->value()) {
		setupShadows();
		drawStuff(true);
		unsetupShadows();
	}

	
	setUBO();
	glBindBufferRange(
		GL_UNIFORM_BUFFER, /*binding point*/0, this->commom_matrices->ubo, 0, this->commom_matrices->size);

	drawSkybox();

	drawPlane();

	drawHeightMapWave();

	DrawParticles();

	drawTiles();

	//loadModel();

	//load2Buffer("Obj/body.obj", 0);
	//
	//load2Buffer("Obj/ulefthand.obj", 1);
	//load2Buffer("Obj/dlefthand.obj", 2);
	//load2Buffer("Obj/lefthand.obj", 3);
	//load2Buffer("Obj/lshouder.obj", 4);
	//
	//load2Buffer("Obj/head.obj", 5);
	//
	//load2Buffer("Obj/urighthand.obj", 6);
	//load2Buffer("Obj/drighthand.obj", 7);
	//load2Buffer("Obj/righthand.obj", 8);
	//load2Buffer("Obj/rshouder.obj", 9);
	//
	//load2Buffer("Obj/dbody.obj", 11);
	//load2Buffer("Obj/back2.obj", 10);
	//
	//load2Buffer("Obj/uleftleg.obj", 12);
	//load2Buffer("Obj/dleftleg.obj", 13);
	//load2Buffer("Obj/leftfoot.obj", 14);
	//
	//load2Buffer("Obj/urightleg.obj", 15);
	//load2Buffer("Obj/drightleg.obj", 16);
	//load2Buffer("Obj/rightfoot.obj", 17);
}

//************************************************************************
//
// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
//========================================================================
void TrainView::
setProjection()
//========================================================================
{
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	// Check whether we use the world camp
	if (tw->worldCam->value())
		arcball.setProjection(false);
	// Or we use the top cam
	else if (tw->topCam->value()) {
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		}
		else {
			he = 110;
			wi = he * aspect;
		}

		// Set up the top camera drop mode to be orthogonal and set
		// up proper projection matrix
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi, wi, -he, he, 200, -200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90, 1, 0, 0);
	}
	// Or do the train view or other view here
	//####################################################################
	// TODO: 
	// put code for train view projection here!	
	//####################################################################
	else {
#ifdef EXAMPLE_SOLUTION
		trainCamView(this, aspect);
#endif
		int splineType = -1;

		splineType = tw->splineBrowser->value();

		Pnt3f qt, orient_t;
		Pnt3f new_qt, new_orient_t;

		int side = t_time * m_pTrack->points.size();
		float t = t_time * m_pTrack->points.size() - side;

		float new_t_time = t_time + (float)1 / m_pTrack->points.size() / (DIVIDE_LINE / 40);
		if (new_t_time > 1.0f)
			new_t_time -= 1.0f;

		int new_side = new_t_time * m_pTrack->points.size();
		float new_t = new_t_time * m_pTrack->points.size() - new_side;

		float T[4]{ pow(t, 3), pow(t, 2), t, 1 };
		float new_T[4]{ pow(new_t, 3), pow(new_t, 2), new_t, 1 };

		float C[4]{ 0 };
		float new_C[4]{ 0 };

		//pos
		Pnt3f cp_pos_p1 = m_pTrack->points[side].pos;
		Pnt3f cp_pos_p2 = m_pTrack->points[(side + 1) % m_pTrack->points.size()].pos;
		Pnt3f cp_pos_p3 = m_pTrack->points[(side + 2) % m_pTrack->points.size()].pos;
		Pnt3f cp_pos_p4 = m_pTrack->points[(side + 3) % m_pTrack->points.size()].pos;
		//orient
		Pnt3f cp_orient_p1 = m_pTrack->points[side].orient;
		Pnt3f cp_orient_p2 = m_pTrack->points[(side + 1) % m_pTrack->points.size()].orient;
		Pnt3f cp_orient_p3 = m_pTrack->points[(side + 2) % m_pTrack->points.size()].orient;
		Pnt3f cp_orient_p4 = m_pTrack->points[(side + 3) % m_pTrack->points.size()].orient;

		//pos
		Pnt3f new_cp_pos_p1 = m_pTrack->points[new_side].pos;
		Pnt3f new_cp_pos_p2 = m_pTrack->points[(new_side + 1) % m_pTrack->points.size()].pos;
		Pnt3f new_cp_pos_p3 = m_pTrack->points[(new_side + 2) % m_pTrack->points.size()].pos;
		Pnt3f new_cp_pos_p4 = m_pTrack->points[(new_side + 3) % m_pTrack->points.size()].pos;

		Pnt3f forward, up;

		switch (splineType)
		{
		case splineType::LINEAR:
			qt = (1 - t) * cp_pos_p1 + t * cp_pos_p2;
			orient_t = (1 - t) * cp_orient_p1 + t * cp_orient_p2;

			new_qt = (1 - new_t) * new_cp_pos_p1 + new_t * new_cp_pos_p2;
			break;
		case splineType::CARDINAL:
			Mult_Q(C, M_cardinal, T);
			qt = cp_pos_p1 * C[0] + cp_pos_p2 * C[1] + cp_pos_p3 * C[2] + cp_pos_p4 * C[3];
			orient_t = cp_orient_p1 * C[0] + cp_orient_p2 * C[1] + cp_orient_p3 * C[2] + cp_orient_p4 * C[3];

			Mult_Q(new_C, M_cardinal, new_T);
			new_qt = new_cp_pos_p1 * new_C[0] + new_cp_pos_p2 * new_C[1] + new_cp_pos_p3 * new_C[2] + new_cp_pos_p4 * new_C[3];
			break;
		case splineType::B_SPLINE:
			Mult_Q(C, M_b_spline, T);
			qt = cp_pos_p1 * C[0] + cp_pos_p2 * C[1] + cp_pos_p3 * C[2] + cp_pos_p4 * C[3];
			orient_t = cp_orient_p1 * C[0] + cp_orient_p2 * C[1] + cp_orient_p3 * C[2] + cp_orient_p4 * C[3];

			Mult_Q(new_C, M_b_spline, new_T);
			new_qt = new_cp_pos_p1 * new_C[0] + new_cp_pos_p2 * new_C[1] + new_cp_pos_p3 * new_C[2] + new_cp_pos_p4 * new_C[3];
			break;
		}

		orient_t.normalize();

		Pnt3f cross_t = (new_qt - qt) * orient_t;
		cross_t.normalize();

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(qt.x, qt.y + 3.0f, qt.z, new_qt.x, new_qt.y, new_qt.z, 0.0f, 1.0f, 0.0f);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(60, aspect, 1.0, 200.0);
	}
}

//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get 
//       colored shadows). this gets called twice per draw 
//       -- once for the objects, once for the shadows
//########################################################################
// TODO: 
// if you have other objects in the world, make sure to draw them
//########################################################################
//========================================================================
void TrainView::drawStuff(bool doingShadows)
{
	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if (((int)i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
	}
	// draw the track
	//####################################################################
	// TODO: 
	// call your own track drawing code
	//####################################################################

	drawTrack(doingShadows);

#ifdef EXAMPLE_SOLUTION
	drawTrack(this, doingShadows);
#endif

	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################
#ifdef EXAMPLE_SOLUTION
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain(this, doingShadows);
#endif

	if (!tw->trainCam->value())
		drawTrain(doingShadows);

	if (!tw->trainCam->value())
	{
		glPushMatrix();
		glScalef(5.0f, 5.0f, 5.0f);
		glTranslatef(20.0f, 7.0f, 20.0f);
		glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
		ferris_wheel.draw(doingShadows, f_time);
		glPopMatrix();

		for (int i = 1; i < 10; ++i)
		{
			trees->draw(glm::vec3(i * 20, 0.0f, 180.0f));
			trees->draw(glm::vec3(180.f, 0.0f, i * 20));

			trees->draw(glm::vec3(-i * 20, 0.0f, 180.0f));
			trees->draw(glm::vec3(180.f, 0.0f, -i * 20));

			trees->draw(glm::vec3(-i * 20, 0.0f, -180.0f));
			trees->draw(glm::vec3(-180.f, 0.0f, -i * 20));

			trees->draw(glm::vec3(i * 20, 0.0f, -180.0f));
			trees->draw(glm::vec3(-180.f, 0.0f, i * 20));
		}

		for (int i = 9; i >= 4; --i)
		{
			trees->draw(glm::vec3(20.0f, 0.0f, i * 20));
			trees->draw(glm::vec3(i * 20, 0.0f, 20.0f));

			trees->draw(glm::vec3(20.0f, 0.0f, -i * 20));
			trees->draw(glm::vec3(-i * 20, 0.0f, 20.0f));

			trees->draw(glm::vec3(-20.0f, 0.0f, -i * 20));
			trees->draw(glm::vec3(-i * 20, 0.0f, -20.0f));

			trees->draw(glm::vec3(-20.0f, 0.0f, i * 20));
			trees->draw(glm::vec3(i * 20, 0.0f, -20.0f));
		}
	}
}

// 
//************************************************************************
//
// * this tries to see which control point is under the mouse
//	  (for when the mouse is clicked)
//		it uses OpenGL picking - which is always a trick
//########################################################################
// TODO: 
//		if you want to pick things other than control points, or you
//		changed how control points are drawn, you might need to change this
//########################################################################
//========================================================================
void TrainView::
doPick()
//========================================================================
{
	// since we'll need to do some GL stuff so we make this window as 
	// active window
	make_current();

	// where is the mouse?
	int mx = Fl::event_x();
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPickMatrix((double)mx, (double)(viewport[3] - my),
		5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100, buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
		glLoadName((GLuint)(i + 1));
		m_pTrack->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3] - 1;
	}
	else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n", selectedCube);
}

void TrainView::setUBO()
{
	float wdt = this->pixel_w();
	float hgt = this->pixel_h();

	glm::mat4 view_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	//HMatrix view_matrix; 
	//this->arcball.getMatrix(view_matrix);

	glm::mat4 projection_matrix;
	glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);
	//projection_matrix = glm::perspective(glm::radians(this->arcball.getFoV()), (GLfloat)wdt / (GLfloat)hgt, 0.01f, 1000.0f);

	glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projection_matrix[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view_matrix[0][0]);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

#define PI 3.14159265
void TrainView::
drawTrack(bool doingShadow)
{
	int splineType = -1;

	splineType = tw->splineBrowser->value();

	float distance = 0;

	for (size_t i = 0; i < m_pTrack->points.size(); ++i)
	{
		//pos
		Pnt3f cp_pos_p1 = m_pTrack->points[i].pos;
		Pnt3f cp_pos_p2 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].pos;
		Pnt3f cp_pos_p3 = m_pTrack->points[(i + 2) % m_pTrack->points.size()].pos;
		Pnt3f cp_pos_p4 = m_pTrack->points[(i + 3) % m_pTrack->points.size()].pos;

		//orient
		Pnt3f cp_orient_p1 = m_pTrack->points[i].orient;
		Pnt3f cp_orient_p2 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].orient;
		Pnt3f cp_orient_p3 = m_pTrack->points[(i + 2) % m_pTrack->points.size()].orient;
		Pnt3f cp_orient_p4 = m_pTrack->points[(i + 3) % m_pTrack->points.size()].orient;

		float percent = 1.0f / DIVIDE_LINE;
		float t = 0;
		Pnt3f qt, orient_t;

		switch (splineType)
		{
		case splineType::LINEAR:
			qt = (1 - t) * cp_pos_p1 + t * cp_pos_p2;
			break;
		case splineType::CARDINAL:
			qt = cp_pos_p2;
			break;
		case splineType::B_SPLINE:
			qt = cp_pos_p1 * (1.0f / 6.0f) + cp_pos_p2 * (4.0f / 6.0f) + cp_pos_p3 * (1.0f / 6.0f);
			break;
		}

		for (size_t j = 0; j < DIVIDE_LINE; ++j)
		{
			Pnt3f qt0 = qt;
			t += percent;

			float G[3][4]{ { cp_pos_p1.x, cp_pos_p2.x, cp_pos_p3.x, cp_pos_p4.x },
						   { cp_pos_p1.y, cp_pos_p2.y, cp_pos_p3.y, cp_pos_p4.y },
						   { cp_pos_p1.z, cp_pos_p2.z, cp_pos_p3.z, cp_pos_p4.z } };

			float T[4]{ pow(t, 3), pow(t, 2), t, 1 };

			float C[4]{ 0 };

			float* dif = new float[4]{ 0 };
			Pnt3f tangent;

			switch (splineType)
			{
			case splineType::LINEAR:
				qt = (1 - t) * cp_pos_p1 + t * cp_pos_p2;
				orient_t = (1 - t) * cp_orient_p1 + t * cp_orient_p2;

				tangent = Pnt3f(cp_pos_p2.x - cp_pos_p1.x, cp_pos_p2.y - cp_pos_p1.y, cp_pos_p2.z - cp_pos_p1.z);
				break;
			case splineType::CARDINAL:
				Mult_Q(C, M_cardinal, T);
				qt = cp_pos_p1 * C[0] + cp_pos_p2 * C[1] + cp_pos_p3 * C[2] + cp_pos_p4 * C[3];
				orient_t = cp_orient_p1 * C[0] + cp_orient_p2 * C[1] + cp_orient_p3 * C[2] + cp_orient_p4 * C[3];

				differential(dif, M_cardinal, t);

				tangent = cp_pos_p1 * dif[0] + cp_pos_p2 * dif[1] + cp_pos_p3 * dif[2] + cp_pos_p4 * dif[3];
				break;
			case splineType::B_SPLINE:
				Mult_Q(C, M_b_spline, T);
				qt = cp_pos_p1 * C[0] + cp_pos_p2 * C[1] + cp_pos_p3 * C[2] + cp_pos_p4 * C[3];
				orient_t = cp_orient_p1 * C[0] + cp_orient_p2 * C[1] + cp_orient_p3 * C[2] + cp_orient_p4 * C[3];

				differential(dif, M_b_spline, t);

				tangent = cp_pos_p1 * dif[0] + cp_pos_p2 * dif[1] + cp_pos_p3 * dif[2] + cp_pos_p4 * dif[3];
				break;
			}

			Pnt3f qt1 = qt;

			if (i == 0 && j == 0)
			{
				totalDistance = 0.0f;
				if (!doingShadow)
					totalDistance += sqrtf(pow(qt1.x - qt0.x, 2) + pow(qt1.y - qt0.y, 2) + pow(qt1.z - qt0.z, 2));
			}

			orient_t.normalize();

			Pnt3f cross_t = (qt1 - qt0) * orient_t;
			cross_t.normalize();
			cross_t = cross_t * 2.5f;

			tangent.normalize();

			int trackType = tw->trackBrowser->value();

			switch (trackType)
			{
			case trackType::SIMPLE:
				glBegin(GL_LINES);
				if (!doingShadow)
					glColor3ub(40, 30, 40);
				glVertex3f(qt0.x, qt0.y, qt0.z);
				glVertex3f(qt1.x, qt1.y, qt1.z);
				glEnd();
				glLineWidth(1);
				break;
			case trackType::PARALLEL:
				glBegin(GL_LINES);
				if (!doingShadow)
					glColor3ub(40, 30, 40);
				glVertex3f(qt0.x + cross_t.x, qt0.y + cross_t.y, qt0.z + cross_t.z);
				glVertex3f(qt1.x + cross_t.x, qt1.y + cross_t.y, qt1.z + cross_t.z);

				glVertex3f(qt0.x - cross_t.x, qt0.y - cross_t.y, qt0.z - cross_t.z);
				glVertex3f(qt1.x - cross_t.x, qt1.y - cross_t.y, qt1.z - cross_t.z);
				glEnd();
				glLineWidth(5);
				break;
			case trackType::ROAD:
				glBegin(GL_LINES);
				if (!doingShadow)
					glColor3ub(40, 30, 40);
				glVertex3f(qt0.x + cross_t.x, qt0.y + cross_t.y, qt0.z + cross_t.z);
				glVertex3f(qt1.x - cross_t.x, qt1.y - cross_t.y, qt1.z - cross_t.z);
				glEnd();
				break;
			}

			//draw Sleeper
			distance += sqrtf(pow(qt1.x - qt0.x, 2) + pow(qt1.y - qt0.y, 2) + pow(qt1.z - qt0.z, 2));

			Pnt3f AC, AB;

			AC = Pnt3f(qt1.x - qt0.x, qt1.y - qt0.y, qt1.z - qt0.z);
			AB = Pnt3f(1.0f, 0.0f, 0.0f);

			float cos_y = (float)(AC.x * AB.x + AC.y * AB.y + AC.z * AB.z)
				/ (abs(sqrt(pow(AC.x, 2) + pow(AC.y, 2) + pow(AC.z, 2))) * abs(sqrt(pow(AB.x, 2) + pow(AB.y, 2) + pow(AB.z, 2))));

			float angle_y = acos(cos_y) * 180.0 / PI;

			if ((AC.x < 0 && AC.z > 0) || (AC.x > 0 && AC.z > 0))
				angle_y = -angle_y;

			AB = Pnt3f(0.0f, 1.0f, 0.0f);

			float cos = (float)(orient_t.x * AB.x + orient_t.y * AB.y + orient_t.z * AB.z)
				/ (abs(sqrt(pow(orient_t.x, 2) + pow(orient_t.y, 2) + pow(orient_t.z, 2))) * abs(sqrt(pow(AB.x, 2) + pow(AB.y, 2) + pow(AB.z, 2))));

			float angle = acos(cos) * 180.0 / PI;

			if (distance > 8)
			{
				distance = 0;

				glPushMatrix();
				glTranslatef(qt1.x, qt1.y, qt1.z);
				glRotatef(angle_y, 0.0f, 1.0f, 0.0f);
				glRotatef(angle, 1.0f, 0.0f, 0.0f);
				drawSleeper(doingShadow);
				glPopMatrix();
			}
		}
	}
}

void TrainView::
Mult_Q(float* C, float M[][4], float* T)
{
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			C[i] += M[j][i] * T[j];
}

void TrainView::
drawTrain(bool doingShadow)
{
	Pnt3f qt, orient_t;

	int splineType = -1;

	splineType = tw->splineBrowser->value();

	int side = t_time * m_pTrack->points.size();
	float t = t_time * m_pTrack->points.size() - side;

	float T[4]{ pow(t, 3), pow(t, 2), t, 1 };

	float C[4]{ 0 };
	float* dif = new float[4]{ 0 };

	//pos
	Pnt3f cp_pos_p1 = m_pTrack->points[side].pos;
	Pnt3f cp_pos_p2 = m_pTrack->points[(side + 1) % m_pTrack->points.size()].pos;
	Pnt3f cp_pos_p3 = m_pTrack->points[(side + 2) % m_pTrack->points.size()].pos;
	Pnt3f cp_pos_p4 = m_pTrack->points[(side + 3) % m_pTrack->points.size()].pos;
	//orient
	Pnt3f cp_orient_p1 = m_pTrack->points[side].orient;
	Pnt3f cp_orient_p2 = m_pTrack->points[(side + 1) % m_pTrack->points.size()].orient;
	Pnt3f cp_orient_p3 = m_pTrack->points[(side + 2) % m_pTrack->points.size()].orient;
	Pnt3f cp_orient_p4 = m_pTrack->points[(side + 3) % m_pTrack->points.size()].orient;

	Pnt3f tangent;

	Pnt3f AC, AB;

	AB = Pnt3f(1.0f, 0.0f, 0.0f);

	float cos = 0.0f, cos_y = 0.0f;

	float angle = 0.0f, angle_y = 0.0f;

	switch (splineType)
	{
	case splineType::LINEAR:
		qt = (1 - t) * cp_pos_p1 + t * cp_pos_p2;
		orient_t = (1 - t) * cp_orient_p1 + t * cp_orient_p2;

		tangent = Pnt3f(cp_pos_p2.x - cp_pos_p1.x, cp_pos_p2.y - cp_pos_p1.y, cp_pos_p2.z - cp_pos_p1.z);
		break;
	case splineType::CARDINAL:
		Mult_Q(C, M_cardinal, T);
		qt = cp_pos_p1 * C[0] + cp_pos_p2 * C[1] + cp_pos_p3 * C[2] + cp_pos_p4 * C[3];
		orient_t = cp_orient_p1 * C[0] + cp_orient_p2 * C[1] + cp_orient_p3 * C[2] + cp_orient_p4 * C[3];

		differential(dif, M_cardinal, t);

		tangent = cp_pos_p1 * dif[0] + cp_pos_p2 * dif[1] + cp_pos_p3 * dif[2] + cp_pos_p4 * dif[3];
		break;
	case splineType::B_SPLINE:
		Mult_Q(C, M_b_spline, T);
		qt = cp_pos_p1 * C[0] + cp_pos_p2 * C[1] + cp_pos_p3 * C[2] + cp_pos_p4 * C[3];
		orient_t = cp_orient_p1 * C[0] + cp_orient_p2 * C[1] + cp_orient_p3 * C[2] + cp_orient_p4 * C[3];

		differential(dif, M_b_spline, t);

		tangent = cp_pos_p1 * dif[0] + cp_pos_p2 * dif[1] + cp_pos_p3 * dif[2] + cp_pos_p4 * dif[3];
		break;
	}

	cos_y = (float)(tangent.x * AB.x + tangent.y * AB.y + tangent.z * AB.z)
		/ (abs(sqrt(pow(tangent.x, 2) + pow(tangent.y, 2) + pow(tangent.z, 2))) * abs(sqrt(pow(AB.x, 2) + pow(AB.y, 2) + pow(AB.z, 2))));

	angle_y = acos(cos_y) * 180.0 / PI;
	if ((tangent.x < 0 && tangent.z > 0) || (tangent.x > 0 && tangent.z > 0))
		angle_y = -angle_y;

	AB = Pnt3f(0.0f, 1.0f, 0.0f);

	cos = (float)(orient_t.x * AB.x + orient_t.y * AB.y + orient_t.z * AB.z)
		/ (abs(sqrt(pow(orient_t.x, 2) + pow(orient_t.y, 2) + pow(orient_t.z, 2))) * abs(sqrt(pow(AB.x, 2) + pow(AB.y, 2) + pow(AB.z, 2))));

	angle = acos(cos) * 180.0 / PI;

	glPushMatrix();
	glTranslatef(qt.x, qt.y + 2.5f, qt.z);
	glRotatef(angle_y, 0.0f, 1.0f, 0.0f);
	glRotatef(angle, 1.0f, 0.0f, 0.0f);
	drawCar(doingShadow);
	glPopMatrix();
}

double* TrainView::
rotate(float m[][3], double* p)
{
	double* n = new double[] {0, 0, 0};
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			n[i] += m[i][j] * p[j];
	return n;
}

float* TrainView::
rotatef(float m[][3], float* p)
{
	float* n = new float[] {0, 0, 0};
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			n[i] += m[i][j] * p[j];

	return n;
}

void TrainView::
drawSleeper(bool doingShadow)
{
	//Down
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(100, 80, 100);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-1.5, 0, 5);
	glTexCoord2f(0.1f, 1.0f);
	glVertex3f(1.5, 0, 5);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(1.5, 0, -5);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-1.5, 0, -5);
	glEnd();

	//Up
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(100, 80, 100);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-1.5, 1, 5);
	glTexCoord2f(0.1f, 1.0f);
	glVertex3f(1.5, 1, 5);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(1.5, 1, -5);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-1.5, 1, -5);
	glEnd();

	//Left
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(100, 80, 100);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-1.5, 0, 5);
	glTexCoord2f(0.1f, 1.0f);
	glVertex3f(-1.5, 1, 5);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(-1.5, 1, -5);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-1.5, 0, -5);
	glEnd();

	//Right
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(100, 80, 100);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(1.5, 0, 5);
	glTexCoord2f(0.1f, 1.0f);
	glVertex3f(1.5, 1, 5);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(1.5, 1, -5);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(1.5, 0, -5);
	glEnd();

	//Front
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(100, 80, 100);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-1.5, 1, 5);
	glTexCoord2f(0.1f, 1.0f);
	glVertex3f(1.5, 1, 5);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(1.5, 0, 5);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-1.5, 0, 5);
	glEnd();

	//Front
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(100, 80, 100);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-1.5, 1, -5);
	glTexCoord2f(0.1f, 1.0f);
	glVertex3f(1.5, 1, -5);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(1.5, 0, -5);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-1.5, 0, -5);
	glEnd();
}

void TrainView::
drawCar(bool doingShadow)
{
	//Up
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(200, 180, 150);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-5, 5, 3);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(5, 5, 3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(5, 5, -3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-5, 5, -3);
	glEnd();

	//Down
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(200, 180, 150);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-5, 0, 3);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(5, 0, 3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(5, 0, -3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-5, 0, -3);
	glEnd();

	//Left
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(200, 180, 150);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-5, 5, 3);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-5, 0, 3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(-5, 0, -3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-5, 5, -3);
	glEnd();

	//Right
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(200, 180, 150);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(5, 5, 3);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(5, 0, 3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(5, 0, -3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(5, 5, -3);
	glEnd();

	//Front
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(200, 180, 150);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(5, 5, 3);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(5, 0, 3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(-5, 0, 3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-5, 5, 3);
	glEnd();

	//Front
	glBegin(GL_QUADS);
	if (!doingShadow)
		glColor3ub(200, 180, 150);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(5, 5, -3);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(5, 0, -3);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(-5, 0, -3);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-5, 5, -3);
	glEnd();

	glPushMatrix();
	glTranslatef(5, 0, 3);
	glRotatef(90, 1.0f, 0.0f, 0.0f);
	drawWheel(doingShadow);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(5, 0, -3);
	glRotatef(90, 1.0f, 0.0f, 0.0f);
	drawWheel(doingShadow);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5, 0, 3);
	glRotatef(90, 1.0f, 0.0f, 0.0f);
	drawWheel(doingShadow);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5, 0, -3);
	glRotatef(90, 1.0f, 0.0f, 0.0f);
	drawWheel(doingShadow);
	glPopMatrix();
}

void  TrainView::
differential(float* C, float M[][4], float t)
{
	float T[4]{ 3 * pow(t, 2), 2 * t, 1, 0 };

	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			C[i] += M[j][i] * T[j];
}

void TrainView::
drawWheel(bool doingShadow)
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float r = 1.5f;

	glBegin(GL_TRIANGLE_FAN);
	if (!doingShadow)
		glColor3ub(0, 0, 0);
	glVertex3f(x, y + 1.0f, z);
	for (int i = 0; i <= 360; ++i)
		glVertex3f(x + r * cos(i * PI / 180.0), y + 1.0f, z + r * sin(i * PI / 180.0));
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	if (!doingShadow)
		glColor3ub(0, 0, 0);
	glVertex3f(x, y - 1.0f, z);
	for (int i = 0; i <= 360; ++i)
		glVertex3f(x + r * cos(i * PI / 180.0), y - 1.0f, z + r * sin(i * PI / 180.0));
	glEnd();

	glBegin(GL_TRIANGLE_STRIP);
	if (!doingShadow)
		glColor3ub(0, 0, 0);
	for (int i = 0; i <= 360; ++i)
	{
		glVertex3f(r * cos(i * PI / 180.0), 1.0f, r * sin(i * PI / 180.0));
		glVertex3f(r * cos(i * PI / 180.0), -1.0f, r * sin(i * PI / 180.0));
	}
	glEnd();
}

unsigned int TrainView::
loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrComponents;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}


void TrainView::
initskyboxShader()
{
	this->skyboxShader = new Shader(PROJECT_DIR "/src/shaders/skybox.vert",
		nullptr, nullptr, nullptr,
		PROJECT_DIR "/src/shaders/skybox.frag");

	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	// skybox VAO
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	//load textures
	vector<std::string> faces;
	faces.push_back("Images/skybox/right.jpg");
	faces.push_back("Images/skybox/left.jpg");
	faces.push_back("Images/skybox/top.jpg");
	faces.push_back("Images/skybox/bottom.jpg");
	faces.push_back("Images/skybox/front.jpg");
	faces.push_back("Images/skybox/back.jpg");
	cubemapTexture = loadCubemap(faces);
}

void TrainView::
drawSkybox()
{
	glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
	skyboxShader->Use();
	glUniform1i(glGetUniformLocation(this->skyboxShader->Program, "skybox"), 0);
	glm::mat4 view;
	glm::mat4 projection;

	glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &projection[0][0]);
	view = glm::mat4(glm::mat3(view)); // remove translation from the view matrix

	glGetFloatv(GL_PROJECTION_MATRIX, &projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(this->skyboxShader->Program, "view"), 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(this->skyboxShader->Program, "projection"), 1, GL_FALSE, &projection[0][0]);

	// skybox cube
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS); // set depth function back to default
}

void TrainView::
AddParticle(Particle ex)
{
	pParticle p;
	p = new Particle;//new particle   
	p->pNext = NULL;        p->pPrev = NULL;
	p->b = ex.b;  p->g = ex.g;  p->r = ex.r;
	p->fade = ex.fade;
	p->life = ex.life;
	p->size = ex.size;
	p->xpos = ex.xpos;
	p->ypos = ex.ypos;
	p->zpos = ex.zpos;
	p->xspeed = ex.xspeed;
	p->yspeed = ex.yspeed;
	p->zspeed = ex.zspeed;
	p->AddCount = ex.AddCount;
	p->AddSpeed = ex.AddSpeed;
	p->bAddParts = ex.bAddParts;
	p->bFire = ex.bFire;
	p->nExpl = ex.nExpl;

	if (!particles)
	{
		particles = p;
		return;
	}
	else
	{
		particles->pPrev = p;
		p->pNext = particles;
		particles = p;
	}
}

void TrainView::
DeleteParticle(pParticle* p)
{
	if (!(*p))
		return;

	if (!(*p)->pNext && !(*p)->pPrev)
	{
		delete(*p);
		*p = NULL;
		return;
	}

	pParticle tmp;
	if (!(*p)->pPrev)
	{
		tmp = (*p);
		*p = (*p)->pNext;
		particles = *p;
		(*p)->pPrev = NULL;
		delete tmp;
		return;
	}

	if (!(*p)->pNext)
	{
		(*p)->pPrev->pNext = NULL;
		delete(*p);
		*p = NULL;
		return;
	}

	tmp = (*p);
	(*p)->pPrev->pNext = (*p)->pNext;
	(*p)->pNext->pPrev = (*p)->pPrev;
	*p = (*p)->pNext;
	delete tmp;
}

void TrainView::
DeleteAll(pParticle* Part)
{
	while ((*Part))
		DeleteParticle(Part);
}

void TrainView::
InitParticle(Particle& ep)
{
	ep.b = float(rand() % 100) / 60.0f;//顏色隨機
	ep.g = float(rand() % 100) / 60.0f;
	ep.r = float(rand() % 100) / 60.0f;
	ep.life = 1.0f;//初始壽命
	ep.fade = 0.005f + float(rand() % 21) / 10000.0f;//衰减速度
	ep.size = 1;//大小  
	ep.xpos = 400.0f - float(rand() % 8001) / 10.0f;//位置 
	ep.ypos = 100.0f;
	ep.zpos = 400.0f - float(rand() % 8001) / 10.0f;

	if (!int(ep.xpos))//x方向速度(z方向相同)
		ep.xspeed = 0.0f;
	else
	{
		if (ep.xpos < 0)
		{
			ep.xspeed = (rand() % int(-ep.xpos)) / 1500.0f;
		}
		else
		{
			ep.xspeed = -(rand() % int(ep.xpos)) / 1500.0f;
		}
	}
	if (!int(ep.zpos))//x方向速度(z方向相同)
		ep.zspeed = 0.0f;
	else
	{
		if (ep.zpos < 0)
		{
			ep.zspeed = (rand() % int(-ep.zpos)) / 1500.0f;
		}
		else
		{
			ep.zspeed = -(rand() % int(ep.zpos)) / 1500.0f;
		}
	}
	ep.yspeed = 0.04f + float(rand() % 11) / 1000.0f;//y方向速度(向上)

	ep.bFire = 1;
	ep.nExpl = 1 + rand() % 6;//粒子效果  
	ep.bAddParts = 1;//設定有尾巴 
	ep.AddCount = 0.0f;
	ep.AddSpeed = 0.2f;
	nOfFires++;//粒子數+1 
	AddParticle(ep);//加入粒子列表

}

void TrainView::
Explosion1(Particle* par)
{
	Particle ep;
	for (int i = 0; i < 100; i++)
	{
		ep.b = float(rand() % 100) / 60.0f;
		ep.g = float(rand() % 100) / 60.0f;
		ep.r = float(rand() % 100) / 60.0f;
		ep.life = 1.0f;
		ep.fade = 0.01f + float(rand() % 31) / 10000.0f;
		ep.size = 0.8f;
		ep.xpos = par->xpos;
		ep.ypos = par->ypos;
		ep.zpos = par->zpos;
		ep.xspeed = 0.02f - float(rand() % 41) / 1000.0f;
		ep.yspeed = 0.02f - float(rand() % 41) / 1000.0f;
		ep.zspeed = 0.02f - float(rand() % 41) / 1000.0f;
		ep.bFire = 0;
		ep.nExpl = 0;
		ep.bAddParts = 0;
		ep.AddCount = 0.0f;
		ep.AddSpeed = 0.0f;
		AddParticle(ep);
	}
}

void TrainView::
Explosion2(Particle* par)
{
	Particle ep;
	for (int i = 0; i < 1000; i++)
	{
		ep.b = par->b;
		ep.g = par->g;
		ep.r = par->r;
		ep.life = 1.0f;
		ep.fade = 0.01f + float(rand() % 31) / 10000.0f;
		ep.size = 0.8f;
		ep.xpos = par->xpos;
		ep.ypos = par->ypos;
		ep.zpos = par->zpos;
		ep.xspeed = 0.02f - float(rand() % 41) / 1000.0f;
		ep.yspeed = 0.02f - float(rand() % 41) / 1000.0f;
		ep.zspeed = 0.02f - float(rand() % 41) / 1000.0f;
		ep.bFire = 0;
		ep.nExpl = 0;
		ep.bAddParts = 0;
		ep.AddCount = 0.0f;
		ep.AddSpeed = 0.0f;
		AddParticle(ep);
	}
}

void TrainView::
Explosion3(Particle* par)
{
	Particle ep;
	float PIAsp = 3.1415926 / 180;
	for (int i = 0; i < 30; i++) {
		float angle = float(rand() % 360) * PIAsp;
		ep.b = par->b;
		ep.g = par->g;
		ep.r = par->r;
		ep.life = 1.5f;
		ep.fade = 0.01f + float(rand() % 31) / 10000.0f;
		ep.size = 0.8f;
		ep.xpos = par->xpos;
		ep.ypos = par->ypos;
		ep.zpos = par->zpos;
		ep.xspeed = (float)sin(angle) * 0.01f;
		ep.yspeed = 0.01f + float(rand() % 11) / 1000.0f;
		ep.zspeed = (float)cos(angle) * 0.01f;
		ep.bFire = 0;
		ep.nExpl = 0;
		ep.bAddParts = 1;
		ep.AddCount = 0.0f;
		ep.AddSpeed = 0.2f;
		AddParticle(ep);
	}
}

void TrainView::
Explosion4(Particle* par)
{
	Particle ep;
	float PIAsp = 3.1415926 / 180;
	for (int i = 0; i < 30; i++) {
		float angle = float(rand() % 360) * PIAsp;
		ep.b = float(rand() % 100) / 60.0f;
		ep.g = float(rand() % 100) / 60.0f;
		ep.r = float(rand() % 100) / 60.0f;
		ep.life = 1.5f;
		ep.fade = 0.01f + float(rand() % 31) / 10000.0f;
		ep.size = 0.8f;
		ep.xpos = par->xpos;
		ep.ypos = par->ypos;
		ep.zpos = par->zpos;
		ep.xspeed = (float)sin(angle) * 0.01f;
		ep.yspeed = 0.01f + float(rand() % 11) / 1000.0f;
		ep.zspeed = (float)cos(angle) * 0.01f;
		ep.bFire = 0;
		ep.nExpl = 0;
		ep.bAddParts = 1;
		ep.AddCount = 0.0f;
		ep.AddSpeed = 0.2f;
		AddParticle(ep);
	}
}

void TrainView::
Explosion5(Particle* par)
{
	Particle ep;
	for (int i = 0; i < 30; i++) {
		ep.b = par->b;
		ep.g = par->g;
		ep.r = par->r;
		ep.life = 0.8f;
		ep.fade = 0.01f + float(rand() % 31) / 10000.0f;
		ep.size = 0.8f;
		ep.xpos = par->xpos;
		ep.ypos = par->ypos;
		ep.zpos = par->zpos;
		ep.xspeed = 0.01f - float(rand() % 21) / 1000.0f;
		ep.yspeed = 0.01f - float(rand() % 21) / 1000.0f;
		ep.zspeed = 0.01f - float(rand() % 21) / 1000.0f;
		ep.bFire = 0;
		ep.nExpl = 7;
		ep.bAddParts = 0;
		ep.AddCount = 0.0f;
		ep.AddSpeed = 0.0f;
		AddParticle(ep);
	}
}

void TrainView::
Explosion6(Particle* par)
{
	Particle ep;
	for (int i = 0; i < 100; i++) {
		ep.b = float(rand() % 100) / 60.0f;
		ep.g = float(rand() % 100) / 60.0f;
		ep.r = float(rand() % 100) / 60.0f;
		ep.life = 0.8f;
		ep.fade = 0.01f + float(rand() % 31) / 10000.0f;
		ep.size = 0.8f;
		ep.xpos = par->xpos;
		ep.ypos = par->ypos;
		ep.zpos = par->zpos;
		ep.xspeed = 0.01f - float(rand() % 21) / 1000.0f;
		ep.yspeed = 0.01f - float(rand() % 21) / 1000.0f;
		ep.zspeed = 0.01f - float(rand() % 21) / 1000.0f;
		ep.bFire = 0;
		ep.nExpl = 7;
		ep.bAddParts = 0;
		ep.AddCount = 0.0f;
		ep.AddSpeed = 0.0f;
		AddParticle(ep);
	}
}

void TrainView::
Explosion7(Particle* par)
{
	Particle ep;
	for (int i = 0; i < 10; i++) {
		ep.b = par->b;
		ep.g = par->g;
		ep.r = par->r;
		ep.life = 0.5f;
		ep.fade = 0.01f + float(rand() % 31) / 10000.0f;
		ep.size = 0.6f;
		ep.xpos = par->xpos;
		ep.ypos = par->ypos;
		ep.zpos = par->zpos;
		ep.xspeed = 0.02f - float(rand() % 41) / 1000.0f;
		ep.yspeed = 0.02f - float(rand() % 41) / 1000.0f;
		ep.zspeed = 0.02f - float(rand() % 41) / 1000.0f;
		ep.bFire = 0;
		ep.nExpl = 0;
		ep.bAddParts = 0;
		ep.AddCount = 0.0f;
		ep.AddSpeed = 0.0f;
		AddParticle(ep);
	}
}

void TrainView::
ProcessParticles()
{
	Tick1 = Tick2;
	Tick2 = GetTickCount();
	DTick = float(Tick2 - Tick1);
	DTick *= 0.5f;
	Particle ep;
	if (nOfFires < MAX_FIRES)
	{
		InitParticle(ep);
	}
	pParticle par = particles;

	while (par) {
		par->life -= par->fade * (float(DTick) * 0.1f);//Particle壽命衰減 
		if (par->life <= 0.05f) 
		{//當壽命小於一定值

			if (par->nExpl) 
			{//爆炸效果
				switch (par->nExpl) 
				{
				case 1:
					Explosion1(par);
					break;
				case 2:
					Explosion2(par);
					break;
				case 3:
					Explosion3(par);
					break;
				case 4:
					Explosion4(par);
					break;
				case 5:
					Explosion5(par);
					break;
				case 6:
					Explosion6(par);
					break;
				case 7:
					Explosion7(par);
					break;
				default:
					break;
				}
			}
			if (par->bFire)
				nOfFires--;
			DeleteParticle(&par);
		}
		else 
		{
			par->xpos += par->xspeed * DTick;   
			par->ypos += par->yspeed * DTick;
			par->zpos += par->zspeed * DTick;   
			par->yspeed -= grav * DTick;
			if (par->bAddParts) {//假如有尾巴
				par->AddCount += 0.01f * DTick;//AddCount變化愈慢，尾巴粒子愈小  
				if (par->AddCount > par->AddSpeed) {//AddSpeed愈大，尾巴粒子愈小  
					par->AddCount = 0;
					ep.b = par->b;  ep.g = par->g;  ep.r = par->r;
					ep.life = par->life - 0.01f;//壽命變短  
					ep.fade = par->fade * 7.0f;//衰减快一些  
					ep.size = 0.6f;//粒子尺寸小一些  
					ep.xpos = par->xpos;  ep.ypos = par->ypos;  ep.zpos = par->zpos;
					ep.xspeed = 0.0f;    ep.yspeed = 0.0f;  ep.zspeed = 0.0f;
					ep.bFire = 0;
					ep.nExpl = 0;
					ep.bAddParts = 0;//尾巴粒子没有尾巴  
					ep.AddCount = 0.0f;
					ep.AddSpeed = 0.0f;
					AddParticle(ep);
				}
			}   par = par->pNext;//更新下一粒子    
		}  
	}
}

void TrainView::
DrawParticles()
{
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTranslatef(0, 0, -60);
	pParticle par;
	par = particles;
	while (par)
	{
		glColor4f(par->r, par->g, par->b, par->life);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2d(1, 1);
		glVertex3f(par->xpos + par->size, par->ypos + par->size, par->zpos);
		glTexCoord2d(0, 1);
		glVertex3f(par->xpos - par->size, par->ypos + par->size, par->zpos);
		glTexCoord2d(1, 0);
		glVertex3f(par->xpos + par->size, par->ypos - par->size, par->zpos);
		glTexCoord2d(0, 0);
		glVertex3f(par->xpos - par->size, par->ypos - par->size, par->zpos);
		glEnd();
		par = par->pNext;
	}
}

void TrainView::
initPlaneShader()
{
	this->planeShader = new Shader{ PROJECT_DIR "/src/shaders/simple.vert",
										nullptr, nullptr, nullptr,
										PROJECT_DIR "/src/shaders/simple.frag" };

	GLfloat vertices[] = {
		//down
		-1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, -1.0f,
		-1.0f, 0.0f, -1.0f,
	};
	GLfloat  normal[] = {
		//down
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
	};

	GLfloat  texture_coordinate[] = {
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
	};

	GLuint element[] = {
		//back
		1, 0, 3,
		3, 2, 1,
	};

	this->plane = new VAO;
	this->plane->element_amount = sizeof(element) / sizeof(GLuint);
	glGenVertexArrays(1, &this->plane->vao);
	glGenBuffers(3, this->plane->vbo);
	glGenBuffers(1, &this->plane->ebo);

	glBindVertexArray(this->plane->vao);

	// Position attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Normal attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	// Texture Coordinate attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate), texture_coordinate, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(2);

	//Element attribute
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element, GL_STATIC_DRAW);

	// Unbind VAO
	glBindVertexArray(0);

	if (!this->planeTexture)
		this->planeTexture = new Texture2D(PROJECT_DIR "/Images/grass.bmp");
}

void TrainView::
drawPlane()
{
	this->planeShader->Use();

	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, this->source_pos);
	model_matrix = glm::scale(model_matrix, glm::vec3(200.0f, 200.0f, 200.0f));

	glm::mat4 view_matrix;
	glm::mat4 projection_matrix;

	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);

	glUniformMatrix4fv(glGetUniformLocation(this->planeShader->Program, "u_view"), 1, GL_FALSE, &view_matrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(this->planeShader->Program, "u_projection"), 1, GL_FALSE, &projection_matrix[0][0]);

	glUniformMatrix4fv(
		glGetUniformLocation(this->planeShader->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);
	glUniform3fv(
		glGetUniformLocation(this->planeShader->Program, "u_color"),
		1,
		&glm::vec3(0.0f, 1.0f, 0.0f)[0]);

	//this->planeTexture->bind(0);
	//glUniform1i(glGetUniformLocation(this->planeShader->Program, "u_texture"), 0);
	//glUniform4fv(glGetUniformLocation(this->planeShader->Program, "plane"), 1, &plane[0]);

	this->planeTexture->bind(0);
	glUniform1i(glGetUniformLocation(this->planeShader->Program, "u_texture"), 0);

	//bind VAO
	glBindVertexArray(this->plane->vao);

	glDrawElements(GL_TRIANGLES, this->plane->element_amount, GL_UNSIGNED_INT, 0);

	//unbind VAO
	glBindVertexArray(0);

	//unbind shader(switch to fixed pipeline)
	glUseProgram(0);
}

void TrainView::
initHeightMapShader()
{
	this->heightMapShader = new Shader{ PROJECT_DIR "/src/shaders/heightMap.vert",
										nullptr, nullptr, nullptr,
										PROJECT_DIR "/src/shaders/heightMap.frag" };

	float size = 0.01f;
	unsigned int width = 2.0f / size;
	unsigned int height = 2.0f / size;

	GLfloat* vertices = new GLfloat[width * height * 4 * 3]();
	GLfloat* normal = new GLfloat[width * height * 4 * 3]();
	GLfloat* texture_coordinate = new GLfloat[width * height * 4 * 2]();
	GLuint* element = new GLuint[width * height * 6]();

	for (int i = 0; i < width * height * 4 * 3; i += 12)
	{
		unsigned int h = i / 12 / width;
		unsigned int w = i / 12 % width;

		vertices[i] = w * size - 1.0f + size;
		vertices[i + 1] = 0.6f;
		vertices[i + 2] = h * size - 1.0f + size;

		vertices[i + 3] = vertices[i] - size;
		vertices[i + 4] = 0.6f;
		vertices[i + 5] = vertices[i + 2];

		vertices[i + 6] = vertices[i + 3];
		vertices[i + 7] = 0.6f;
		vertices[i + 8] = vertices[i + 5] - size;

		vertices[i + 9] = vertices[i];
		vertices[i + 10] = 0.6f;
		vertices[i + 11] = vertices[i + 8];
	}

	for (int i = 0; i < width * height * 4 * 3; i += 3)
	{
		normal[i] = 0.0f;
		normal[i + 1] = 1.0f;
		normal[i + 2] = 0.0f;
	}

	for (int i = 0, j = 0; i < width * height * 4 * 2; i += 8, ++j)
	{
		int ii = i / (4 * 2), jj = i / (height * +4 * 2);
		texture_coordinate[i] = 0.005 + ii * 0.005;
		texture_coordinate[i + 1] = 0.005 + jj * 0.005;

		texture_coordinate[i + 2] = 0.0f + ii * 0.005;
		texture_coordinate[i + 3] = 0.005 + jj * 0.005;

		texture_coordinate[i + 4] = 0.0f + ii * 0.005;
		texture_coordinate[i + 5] = 0.0f + jj * 0.005;

		texture_coordinate[i + 6] = 0.005 + ii * 0.005;
		texture_coordinate[i + 7] = 0.0f + jj * 0.005;
	}

	for (int i = 0, j = 0; i < width * height * 6; i += 6, j += 4)
	{
		element[i] = j + 1;
		element[i + 1] = j;
		element[i + 2] = j + 3;

		element[i + 3] = element[i + 2];
		element[i + 4] = j + 2;
		element[i + 5] = element[i];
	}

	this->heightMap = new VAO;
	this->heightMap->element_amount = width * height * 6;
	glGenVertexArrays(1, &this->heightMap->vao);
	glGenBuffers(3, this->heightMap->vbo);
	glGenBuffers(1, &this->heightMap->ebo);

	glBindVertexArray(this->heightMap->vao);

	// Position attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->heightMap->vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, width * height * 4 * 3 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Normal attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->heightMap->vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, width * height * 4 * 3 * sizeof(GLfloat), normal, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	// Texture Coordinate attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->heightMap->vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, width * height * 4 * 2 * sizeof(GLfloat), texture_coordinate, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(2);

	//Element attribute
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->heightMap->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, width * height * 6 * sizeof(GLuint), element, GL_STATIC_DRAW);

	// Unbind VAO
	glBindVertexArray(0);

	for (int i = 0; i < 200; ++i)
	{
		std::string name;
		if (i < 10)
			name = "00" + std::to_string(i);
		else if (i < 100)
			name = "0" + std::to_string(i);
		else
			name = std::to_string(i);

		this->heightMapTexture.push_back(Texture2D(("Images/waves5/" + name + ".png").c_str()));
	}
}

void TrainView::
drawHeightMapWave()
{
	glEnable(GL_BLEND);

	this->heightMapShader->Use();

	glm::mat4 model_matrix = glm::mat4();
	//model_matrix = glm::translate(model_matrix, this->source_pos);
	model_matrix = glm::translate(model_matrix, pos);
	model_matrix = glm::scale(model_matrix, scal);

	glUniformMatrix4fv(
		glGetUniformLocation(this->heightMapShader->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);
	glUniform3fv(
		glGetUniformLocation(this->heightMapShader->Program, "u_color"),
		1,
		&glm::vec3(0.0f, 1.0f, 0.0f)[0]);

	heightMapTexture[heightMapIndex].bind(0);
	glUniform1i(glGetUniformLocation(this->heightMapShader->Program, "u_texture"), 0);
	this->planeTexture->bind(1);
	glUniform1i(glGetUniformLocation(this->heightMapShader->Program, "tiles"), 1);

	//glUniform1f(glGetUniformLocation(this->heightMapShader->Program, "amplitude"), tw->amplitude->value());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, this->cubemapTexture);
	glUniform1i(glGetUniformLocation(this->heightMapShader->Program, "skyBox"), 0);

	glUniform1f(glGetUniformLocation(this->heightMapShader->Program, "time"), t_time);

	GLfloat* view_matrix = new GLfloat[16]();

	glGetFloatv(GL_MODELVIEW_MATRIX, view_matrix);

	view_matrix = inverse(view_matrix);

	this->cameraPosition = glm::vec3(view_matrix[12], view_matrix[13], view_matrix[14]);
	glUniform3fv(glGetUniformLocation(this->heightMapShader->Program, "camera"), 1, &cameraPosition[0]);

	//bind VAO
	glBindVertexArray(this->heightMap->vao);

	glDrawElements(GL_TRIANGLES, this->heightMap->element_amount, GL_UNSIGNED_INT, 0);

	for (int i = 0; i < allDrop.size(); ++i)
	{
		if (t_time - allDrop[i].time > allDrop[i].keepTime)
		{
			allDrop.erase(allDrop.begin() + i);
			--i;
			continue;
		}

		glUniform2f(glGetUniformLocation(this->heightMapShader->Program, "dropPoint"), allDrop[i].point.x, allDrop[i].point.y);
		std::cout << allDrop[i].point.x << " " << allDrop[i].point.y << std::endl;
		glUniform1f(glGetUniformLocation(this->heightMapShader->Program, "dropTime"), allDrop[i].time);
		glUniform1f(glGetUniformLocation(this->heightMapShader->Program, "interactiveRadius"), allDrop[i].radius);

		glDrawElements(GL_TRIANGLES, this->heightMap->element_amount, GL_UNSIGNED_INT, 0);
	}

	//unbind VAO
	glBindVertexArray(0);

	//unbind shader(switch to fixed pipeline)
	glUseProgram(0);

	glDisable(GL_BLEND);
}

void TrainView::
addDrop(float radius, float keepTime)
{
	glBindFramebuffer(GL_FRAMEBUFFER, interactiveFrameBuffer);
	glBindTexture(GL_TEXTURE_2D, interactiveTextureBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, interactiveRenderBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	interactiveFrameShader->Use();

	glm::mat4 view_matrix;
	glm::mat4 projection_matrix;

	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);

	glUniformMatrix4fv(glGetUniformLocation(this->interactiveFrameShader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(this->interactiveFrameShader->Program, "projection"), 1, GL_FALSE, &projection_matrix[0][0]);

	glm::mat4 model_matrix = glm::mat4(1.0f);
	model_matrix = glm::translate(model_matrix, glm::vec3(0.0f, 0.0f, 0.0f));
	model_matrix = glm::scale(model_matrix, glm::vec3(100.0f, 100.0f, 100.0f));

	glUniformMatrix4fv(
		glGetUniformLocation(this->interactiveFrameShader->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);

	glBindVertexArray(this->heightMap->vao);
	glDrawElements(GL_TRIANGLES, this->heightMap->element_amount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glm::vec3 uv;
	glReadPixels(Fl::event_x(), h() - Fl::event_y(), 1, 1, GL_RGB, GL_FLOAT, &uv[0]);

	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	if (uv.b != 1.0f)
		allDrop.push_back(Drop(glm::vec2(uv.x, uv.y), this->t_time, radius, keepTime));
}

GLfloat* TrainView::
inverse(GLfloat* m)
{
	GLfloat* inv = new GLfloat[16]();
	float det;

	int i;

	inv[0] = m[5] * m[10] * m[15] -
		m[5] * m[11] * m[14] -
		m[9] * m[6] * m[15] +
		m[9] * m[7] * m[14] +
		m[13] * m[6] * m[11] -
		m[13] * m[7] * m[10];

	inv[4] = -m[4] * m[10] * m[15] +
		m[4] * m[11] * m[14] +
		m[8] * m[6] * m[15] -
		m[8] * m[7] * m[14] -
		m[12] * m[6] * m[11] +
		m[12] * m[7] * m[10];

	inv[8] = m[4] * m[9] * m[15] -
		m[4] * m[11] * m[13] -
		m[8] * m[5] * m[15] +
		m[8] * m[7] * m[13] +
		m[12] * m[5] * m[11] -
		m[12] * m[7] * m[9];

	inv[12] = -m[4] * m[9] * m[14] +
		m[4] * m[10] * m[13] +
		m[8] * m[5] * m[14] -
		m[8] * m[6] * m[13] -
		m[12] * m[5] * m[10] +
		m[12] * m[6] * m[9];

	inv[1] = -m[1] * m[10] * m[15] +
		m[1] * m[11] * m[14] +
		m[9] * m[2] * m[15] -
		m[9] * m[3] * m[14] -
		m[13] * m[2] * m[11] +
		m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] -
		m[0] * m[11] * m[14] -
		m[8] * m[2] * m[15] +
		m[8] * m[3] * m[14] +
		m[12] * m[2] * m[11] -
		m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] +
		m[0] * m[11] * m[13] +
		m[8] * m[1] * m[15] -
		m[8] * m[3] * m[13] -
		m[12] * m[1] * m[11] +
		m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] -
		m[0] * m[10] * m[13] -
		m[8] * m[1] * m[14] +
		m[8] * m[2] * m[13] +
		m[12] * m[1] * m[10] -
		m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] -
		m[1] * m[7] * m[14] -
		m[5] * m[2] * m[15] +
		m[5] * m[3] * m[14] +
		m[13] * m[2] * m[7] -
		m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] +
		m[0] * m[7] * m[14] +
		m[4] * m[2] * m[15] -
		m[4] * m[3] * m[14] -
		m[12] * m[2] * m[7] +
		m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] -
		m[0] * m[7] * m[13] -
		m[4] * m[1] * m[15] +
		m[4] * m[3] * m[13] +
		m[12] * m[1] * m[7] -
		m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] +
		m[0] * m[6] * m[13] +
		m[4] * m[1] * m[14] -
		m[4] * m[2] * m[13] -
		m[12] * m[1] * m[6] +
		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
		m[1] * m[7] * m[10] +
		m[5] * m[2] * m[11] -
		m[5] * m[3] * m[10] -
		m[9] * m[2] * m[7] +
		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
		m[0] * m[7] * m[10] -
		m[4] * m[2] * m[11] +
		m[4] * m[3] * m[10] +
		m[8] * m[2] * m[7] -
		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
		m[0] * m[7] * m[9] +
		m[4] * m[1] * m[11] -
		m[4] * m[3] * m[9] -
		m[8] * m[1] * m[7] +
		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
		m[0] * m[6] * m[9] -
		m[4] * m[1] * m[10] +
		m[4] * m[2] * m[9] +
		m[8] * m[1] * m[6] -
		m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	if (det == 0)
		return false;

	det = 1.0 / det;

	for (i = 0; i < 16; i++)
		inv[i] = inv[i] * det;

	return inv;
}

void TrainView::
initTilesShader()
{
	this->tilesShader = new Shader(PROJECT_DIR "/src/shaders/tiles.vert",
		nullptr, nullptr, nullptr,
		PROJECT_DIR "/src/shaders/tiles.frag");

	GLfloat  vertices[] = {
		// back
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		//left
		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,

		//front
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,

		//right
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,

		//down
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,

		//up
		//- 1.0f, 0.6f, 1.0f,
		//1.0f, 0.6f, 1.0f,
		//1.0f, 0.6f, -1.0f,
		//-1.0f, 0.6f, -1.0f
	};
	GLfloat  normal[] = {
		//back
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,

		//left
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		//front
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,

		//right
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,

		//down
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,

		//up
		//0.0f, 1.0f, 0.0f,
		//0.0f, 1.0f, 0.0f,
		//0.0f, 1.0f, 0.0f,
		//0.0f, 1.0f, 0.0f
	};
	GLfloat  texture_coordinate[] = {
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,

		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,

		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,

		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,

		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
	};
	GLuint element[] = {
		//back
		1, 0, 3,
		3, 2, 1,

		//left
		5, 4, 7,
		7, 6, 5,

		//front
		9, 8, 11,
		11, 10, 9,

		//right
		13, 12, 15,
		15, 14, 13,

		//down
		17, 16, 19,
		19, 18, 17,
	};

	this->tiles = new VAO;
	this->tiles->element_amount = sizeof(element) / sizeof(GLuint);
	glGenVertexArrays(1, &this->tiles->vao);
	glGenBuffers(3, this->tiles->vbo);
	glGenBuffers(1, &this->tiles->ebo);

	glBindVertexArray(this->tiles->vao);

	// Position attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->tiles->vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Normal attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->tiles->vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	// Texture Coordinate attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->tiles->vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate), texture_coordinate, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(2);

	//Element attribute
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->tiles->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element, GL_STATIC_DRAW);

	// Unbind VAO
	glBindVertexArray(0);

	if (!this->tilesTexture)
		this->tilesTexture = new Texture2D(PROJECT_DIR "/Images/dolphin.jpg");
}

void TrainView::
drawTiles()
{
	this->tilesShader->Use();

	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, this->source_pos);
	model_matrix = glm::translate(model_matrix, glm::vec3(0.0f, 20.0f, 0.0f));
	model_matrix = glm::translate(model_matrix, pos);
	model_matrix = glm::scale(model_matrix, scal);	

	//if (reflection)
	//	model_matrix = glm::scale(model_matrix, glm::vec3(1, 1, 1));

	glm::mat4 view_matrix;
	glm::mat4 projection_matrix;

	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);

	glUniformMatrix4fv(glGetUniformLocation(this->tilesShader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(this->tilesShader->Program, "projection"), 1, GL_FALSE, &projection_matrix[0][0]);

	glUniformMatrix4fv(
		glGetUniformLocation(this->tilesShader->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);
	glUniform3fv(
		glGetUniformLocation(this->tilesShader->Program, "u_color"),
		1,
		&glm::vec3(0.0f, 1.0f, 0.0f)[0]);
	this->tilesTexture->bind(0);
	glUniform1i(glGetUniformLocation(this->tilesShader->Program, "u_texture"), 0);
	//glUniform4fv(glGetUniformLocation(this->tilesShader->Program, "plane"), 1, &plane[0]);

	//bind VAO
	glBindVertexArray(this->tiles->vao);

	glDrawElements(GL_TRIANGLES, this->tiles->element_amount, GL_UNSIGNED_INT, 0);

	//unbind VAO
	glBindVertexArray(0);

	//unbind shader(switch to fixed pipeline)
	glUseProgram(0);
}

void TrainView::
load2Buffer(char* obj, int i) {
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals; // Won't be used at the moment.
	std::vector<unsigned int> materialIndices;

	bool res = loadOBJ(obj, vertices, uvs, normals, faces[i], mtls[i]);
	if (!res) printf("load failed\n");

	//glUseProgram(program);

	glGenBuffers(1, &VBOs[i]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[i]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	vertices_size[i] = vertices.size();

	//(buffer type,data起始位置,data size,data first ptr)
	//vertices_size[i] = glm_model->numtriangles;

	//printf("vertices:%d\n",vertices_size[);

	glGenBuffers(1, &uVBOs[i]);
	glBindBuffer(GL_ARRAY_BUFFER, uVBOs[i]);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
	uvs_size[i] = uvs.size();

	glGenBuffers(1, &nVBOs[i]);
	glBindBuffer(GL_ARRAY_BUFFER, nVBOs[i]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
	normals_size[i] = normals.size();
}

//bool TrainView::
//loadModel()
//{
//	//std::ifstream file;
//	//file.open("/src/Obj/armadillo.obj", std::ios::in);
//	////file.read("/src/Obj/armadillo.obj");
//	//if (!file)
//	//{
//	//	std::cout << "Impossible to open the file!" << std::endl;
//	//	return false;
//	//}
//	//
//	//while (1)
//	//{
//	//	char lineHeader[128];
//	//	//int res = fscanf(file, "%s", lineHeader);
//	//	//if (res == EOF)
//	//		break;
//	//}
//	return true;
//}