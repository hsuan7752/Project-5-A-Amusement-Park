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
class Aquarium;
class SimpleController;

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

#include <AL/alut.h>
#include <vector>
#include <string>

// this uses the old ArcBall Code
#include "Utilities/ArcBallCam.H"
#include "Utilities/Pnt3f.H"

#include "FerrisWheels.H"
#include "Tree.H"
#include "Aquarium.H"
#include "objloader.hpp"

//#include <fstream>

typedef struct tag_PARTICLE
{
	GLfloat xpos;//(xpos,ypos,zpos)為particle的position
	GLfloat ypos;
	GLfloat zpos;
	GLfloat xspeed;//(xspeed,yspeed,zspeed)為particle的speed 
	GLfloat yspeed;
	GLfloat zspeed;
	GLfloat r;//(r,g,b)為particle的color
	GLfloat g;
	GLfloat b;
	GLfloat life;// particle的壽命 
	GLfloat fade;// particle的衰減速度
	GLfloat size;// particle的大小  
	GLbyte    bFire;
	GLbyte    nExpl;//哪種particle效果  
	GLbyte    bAddParts;// particle是否含有尾巴
	GLfloat   AddSpeed;//尾巴粒子的加速度  
	GLfloat   AddCount;//尾巴粒子的增加量  
	tag_PARTICLE* pNext;//下一particle 
	tag_PARTICLE* pPrev;//上一particle

} Particle, * pParticle;

struct Drop
{
	Drop(glm::vec2 p, float t, float r, float k)
		:point(p), time(t), radius(r), keepTime(k)
	{
	}

	glm::vec2 point;
	float time;
	float radius;
	float keepTime;
};

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

	//set ubo
	void setUBO();

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

	void	AddParticle(Particle ex);

	void	DeleteParticle(pParticle* p);

	void	DeleteAll(pParticle* Part);

	void	InitParticle(Particle& ep);

	void	Explosion1(Particle* par);

	void	Explosion2(Particle* par);

	void	Explosion3(Particle* par);

	void	Explosion4(Particle* par);

	void	Explosion5(Particle* par);

	void	Explosion6(Particle* par);

	void	Explosion7(Particle* par);

	void	ProcessParticles();

	void	DrawParticles();

	void	initPlaneShader();

	void	drawPlane();

	void	initHeightMapShader();

	void	drawHeightMapWave();

	void	addDrop(float radius, float keepTime);

	void	initTilesShader();

	void	drawTiles();

	void	load2Buffer(char* obj, int i);

	//bool	loadModel();

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

	Shader*			fireworksShader = nullptr;
	VAO*			fireworksVBO;
	GLuint*			fireworksTexture;

	pParticle		particles = nullptr;
	UINT			nOfFires;

	UINT			Tick1, Tick2;
	float			DTick;
	GLfloat			grav = 0.00003f;
	GLuint			textureID;	

#define MAX_PARTICLES 1000
#define MAX_FIRES 5

	Shader*			planeShader = nullptr;
	VAO*			plane = nullptr;
	Texture2D*		planeTexture = nullptr;

	//OpenAL
	glm::vec3 source_pos;
	glm::vec3 listener_pos;
	ALCdevice* device = nullptr;
	ALCcontext* context = nullptr;
	ALuint source;
	ALuint buffer;

	UBO* commom_matrices = nullptr;

	Shader* heightMapShader = nullptr;
	VAO* heightMap = nullptr;
	std::vector<Texture2D> heightMapTexture;
		
	unsigned int		heightMapIndex = 0;
	float				moveFactor = 0.0f;
	float				WAVE_SPEED = 0.03f;
	glm::vec3			cameraPosition;
	glm::vec3			lightColor;
	glm::vec3			lightPosition;

	std::vector<Drop> allDrop;
	Shader* interactiveFrameShader = nullptr;
	unsigned int interactiveFrameBuffer;
	unsigned int interactiveTextureBuffer;
	unsigned int interactiveRenderBuffer;
	unsigned int interactiveQuadVAO;
	unsigned int interactiveQuadVBO;

	Tree*				trees;

	Shader* tilesShader = nullptr;
	VAO* tiles = nullptr;
	Texture2D* tilesTexture = nullptr;

	glm::vec3 scal = glm::vec3(50.0f, 20.0f, 50.0f);
	glm::vec3 pos = glm::vec3(-100.0f, 0.0f, -100.0f);

public:
	GLfloat* inverse(GLfloat* m);

public:
#define PARTSNUM 18

	//GLuint VAO;
	GLuint VBO;
	GLuint uVBO;
	GLuint nVBO;
	GLuint mVBO;
	//GLuint UBO;
	GLuint VBOs[PARTSNUM];
	GLuint uVBOs[PARTSNUM];
	GLuint nVBOs[PARTSNUM];
	GLuint program;
	int pNo;

	int vertices_size[PARTSNUM];
	int uvs_size[PARTSNUM];
	int normals_size[PARTSNUM];
	int materialCount[PARTSNUM];

	std::vector<std::string> mtls[PARTSNUM];//use material
	std::vector<unsigned int> faces[PARTSNUM];//face count
};

