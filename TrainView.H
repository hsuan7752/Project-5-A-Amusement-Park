/************************************************************************
	 File:        TrainView.H

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

#pragma once

#include "RenderUtilities/BufferObject.h"
#include "RenderUtilities/Shader.h"
#include "RenderUtilities/Texture.h"
#include "RenderUtilities/WaterFrameBuffer.H"

// Preclarify for preventing the compiler error
class TrainWindow;
class CTrack;
class FerrisWheel;

//#######################################################################
// TODO
// You might change the TrainView in order to add different objects to
// be drawn, or adjust the lighting, or ...
//#######################################################################


// The TrainView also needs to handle its user interface events.
// since this is an FLTk Window, we need to include FlTk Headers
// the pragmas turn off the warnings from FlTk - our code should have no warnings,
// but their code doesn't meet that standard
#pragma warning(push)
#pragma warning(disable:4312)
#pragma warning(disable:4311)
#include <Fl/Fl_Gl_Window.h>
#pragma warning(pop)

// this uses the old ArcBall Code
#include "Utilities/ArcBallCam.H"
#include "Utilities/Pnt3f.H"

#include "FerrisWheels.H"

class TrainView : public Fl_Gl_Window
{
public:
	// note that we keep the "standard widget" constructor arguments
	TrainView(int x, int y, int w, int h, const char* l = 0);

	// overrides of important window things
	virtual int handle(int);
	virtual void draw();

	// all of the actual drawing happens in this routine
	// it has to be encapsulated, since we draw differently if
	// we're drawing shadows (no colors, for example)
	void	drawStuff(bool doingShadows = false);

	// setup the projection - assuming that the projection stack has been
	// cleared for you
	void	setProjection();

	// Reset the Arc ball control
	void	resetArcball();

	// pick a point (for when the mouse goes down)
	void	doPick();

	void	drawTrack(bool doingShadow);

	void	Mult_Q(float* C, float M[][4], float* T);

	void	drawTrain(bool doingShadow);

	double* rotate(float m[][3], double* p);

	float* rotatef(float m[][3], float* p);

	void	drawSleeper(bool doingShadow);

	void	drawCar(bool doingShadow);

	void	differential(float* C, float M[][4], float t);

	void	drawWheel(bool doingShadow);

	unsigned int loadCubemap(std::vector<std::string> faces);
	
	void	initskyboxShader();
	
	void	drawSkybox();

public:
	ArcBallCam		arcball;			// keep an ArcBall for the UI
	int				selectedCube;  // simple - just remember which cube is selected

	TrainWindow*	tw;				// The parent of this display window
	CTrack*			m_pTrack;		// The track of the entire scene
	float			t_time = 0.0f;
	float			s_time = 0.0f;
	unsigned int	DIVIDE_LINE = 500;
	float			totalDistance = 0.0f;
	FerrisWheel		ferris_wheel;

	float			f_time = 0.0f;

	Shader*			skyboxShader = nullptr;
	Texture2D*		skyboxTexture = nullptr;
	
	unsigned int	skyboxVAO;
	unsigned int	skyboxVBO;
	unsigned int	cubemapTexture;
};