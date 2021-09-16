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
#include "GL/glu.h"
#include <math.h>

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb-master/stb_image.h"
//#include <iostream>

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
		//initiailize VAO, VBO, Shader...
		//if (!skyboxShader)
		//	initskyboxShader();
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

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
	}
	else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
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



	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	glDisable(GL_LIGHTING);
	drawFloor(200, 10);


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

	if (tw->spotLT->value())
	{

	}
	else
		glDisable(GL_LIGHT1);

	if (tw->pointLT->value())
	{
		float yellowAmbientDiffuse[] = { 1.0f, 1.0f, 0.0f, 1.0f };
		float position[] = { 0.0f, 100.0f, 0.0f, 1.0f };

		glEnable(GL_LIGHT1);
		glLightfv(GL_LIGHT1, GL_AMBIENT, yellowAmbientDiffuse);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowAmbientDiffuse);
		glLightfv(GL_LIGHT1, GL_POSITION, position);
	}
	else
		glDisable(GL_LIGHT1);
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
		//orient
		Pnt3f new_cp_orient_p1 = m_pTrack->points[new_side].orient;
		Pnt3f new_cp_orient_p2 = m_pTrack->points[(new_side + 1) % m_pTrack->points.size()].orient;
		Pnt3f new_cp_orient_p3 = m_pTrack->points[(new_side + 2) % m_pTrack->points.size()].orient;
		Pnt3f new_cp_orient_p4 = m_pTrack->points[(new_side + 3) % m_pTrack->points.size()].orient;

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
		glTranslatef(0.0f, 7.0f, 0.0f);
		ferris_wheel.draw(doingShadows);
		glPopMatrix();

		//drawSkybox();
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

//unsigned int TrainView::
//loadCubemap(std::vector<std::string> faces)
//{
//	unsigned int textureID;
//	glGenTextures(1, &textureID);
//	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
//
//	int width, height, nrComponents;
//	for (unsigned int i = 0; i < faces.size(); i++)
//	{
//		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
//		if (data)
//		{
//			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
//			stbi_image_free(data);
//		}
//		else
//		{
//			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
//			stbi_image_free(data);
//		}
//	}
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
//
//	return textureID;
//}
//
//
//void TrainView::
//initskyboxShader()
//{
//	this->skyboxShader = new Shader(PROJECT_DIR "/src/shaders/skybox.vert",
//		nullptr, nullptr, nullptr,
//		PROJECT_DIR "/src/shaders/skybox.frag");
//
//	float skyboxVertices[] = {
//		// positions          
//		-1.0f,  1.0f, -1.0f,
//		-1.0f, -1.0f, -1.0f,
//		 1.0f, -1.0f, -1.0f,
//		 1.0f, -1.0f, -1.0f,
//		 1.0f,  1.0f, -1.0f,
//		-1.0f,  1.0f, -1.0f,
//
//		-1.0f, -1.0f,  1.0f,
//		-1.0f, -1.0f, -1.0f,
//		-1.0f,  1.0f, -1.0f,
//		-1.0f,  1.0f, -1.0f,
//		-1.0f,  1.0f,  1.0f,
//		-1.0f, -1.0f,  1.0f,
//
//		 1.0f, -1.0f, -1.0f,
//		 1.0f, -1.0f,  1.0f,
//		 1.0f,  1.0f,  1.0f,
//		 1.0f,  1.0f,  1.0f,
//		 1.0f,  1.0f, -1.0f,
//		 1.0f, -1.0f, -1.0f,
//
//		-1.0f, -1.0f,  1.0f,
//		-1.0f,  1.0f,  1.0f,
//		 1.0f,  1.0f,  1.0f,
//		 1.0f,  1.0f,  1.0f,
//		 1.0f, -1.0f,  1.0f,
//		-1.0f, -1.0f,  1.0f,
//
//		-1.0f,  1.0f, -1.0f,
//		 1.0f,  1.0f, -1.0f,
//		 1.0f,  1.0f,  1.0f,
//		 1.0f,  1.0f,  1.0f,
//		-1.0f,  1.0f,  1.0f,
//		-1.0f,  1.0f, -1.0f,
//
//		-1.0f, -1.0f, -1.0f,
//		-1.0f, -1.0f,  1.0f,
//		 1.0f, -1.0f, -1.0f,
//		 1.0f, -1.0f, -1.0f,
//		-1.0f, -1.0f,  1.0f,
//		 1.0f, -1.0f,  1.0f
//	};
//
//	// skybox VAO
//	glGenVertexArrays(1, &skyboxVAO);
//	glGenBuffers(1, &skyboxVBO);
//	glBindVertexArray(skyboxVAO);
//	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
//	glEnableVertexAttribArray(0);
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
//
//	//load textures
//	vector<std::string> faces;
//	faces.push_back("Images/skybox/right.jpg");
//	faces.push_back("Images/skybox/left.jpg");
//	faces.push_back("Images/skybox/top.jpg");
//	faces.push_back("Images/skybox/bottom.jpg");
//	faces.push_back("Images/skybox/front.jpg");
//	faces.push_back("Images/skybox/back.jpg");
//	cubemapTexture = loadCubemap(faces);
//}
//
//void TrainView::
//drawSkybox()
//{
//	glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
//	skyboxShader->Use();
//	glUniform1i(glGetUniformLocation(this->skyboxShader->Program, "skybox"), 0);
//	glm::mat4 view;
//	glm::mat4 projection;
//
//	glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
//	glGetFloatv(GL_PROJECTION_MATRIX, &projection[0][0]);
//	view = glm::mat4(glm::mat3(view)); // remove translation from the view matrix
//
//	glGetFloatv(GL_PROJECTION_MATRIX, &projection[0][0]);
//	glUniformMatrix4fv(glGetUniformLocation(this->skyboxShader->Program, "view"), 1, GL_FALSE, &view[0][0]);
//	glUniformMatrix4fv(glGetUniformLocation(this->skyboxShader->Program, "projection"), 1, GL_FALSE, &projection[0][0]);
//
//	// skybox cube
//	glBindVertexArray(skyboxVAO);
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
//	glDrawArrays(GL_TRIANGLES, 0, 36);
//	glBindVertexArray(0);
//	glDepthFunc(GL_LESS); // set depth function back to default
//}