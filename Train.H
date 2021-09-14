#pragma once
#include "Utilities/Pnt3f.H"

class TrainWindow;
class TrainView;
class CTrack;

class Train
{
public:
	Train(Pnt3f pos, Pnt3f orient, float* vec, bool doingShadow);

	Train(Pnt3f pos);

	void			add(Pnt3f pos, Pnt3f orient, float* vec, bool doingShadow);

	void			del();

	void			draw(bool doingShadow, float* vec);	

	void			firstCar(bool doingShadow);

	float*			rotatef(float m[][3], float* p);

private:
	Pnt3f			pos;

	Pnt3f			orient;

	Train*			next_car = nullptr;	

	TrainView*		trainView;

	TrainWindow*	tw;

	CTrack*			m_pTrack;
};