#pragma once
#define GLUT_DISABLE_ATEXIT_HACK
#include <gl/GLU.h>

class Particle {
	bool is_forever;//ッネ
	bool has_tex;//瞶┪肅︹
	float x, y;//竚
	float size_x;//
	float size_y;
	unsigned int texture;//瞶
	float speed_x;//硉
	float speed_y;
	float acc_x;//硉
	float acc_y;
	float life;//ネ㏑
	float angle;//à
	unsigned char color[3];//肅︹
	friend class emitter;
	void draw(GLuint texture);//酶采瞶
	void draw();//酶采肅︹
	void show();
public:
	Particle();
	Particle(float _size_x, float _size_y,
		float _speed_x, float _speed_y,
		float _acc_x, float _acc_y,
		float _life, float _angle, unsigned int _texture, bool _is_forever);
	Particle(float _size_x, float _size_y,
		float _speed_x, float _speed_y,
		float _acc_x, float _acc_y,
		float _life, float _angle, unsigned char* _color, bool _is_forever);
};

class Emitter {
	float x1, y1, x2, y2; //祇甮竟竚
	int speed;//祇甮硉瞯
	Particle** p;//祇甮采
	Particle* (*f)(); //﹍て采ㄧ计皐
public:
	void emit(Particle* (init)());
	void update();
	Emitter(int _speed, float _x1,
		float _x2, float _y1, float _y2);
};