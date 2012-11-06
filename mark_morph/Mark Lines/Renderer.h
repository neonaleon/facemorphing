#pragma once
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <cv.h>
#include <highgui.h>

using namespace cv;

class CMarkUI;

class CRenderer
{
private:
	CMarkUI *m_pImageA, *m_pImageB;
	int m_width, m_height;
	GLuint m_morphProg;
	GLuint m_texA, m_texB, m_texLineA, m_texLineB, m_morphedTexObj;
	GLuint m_fbo;

private:
	void initGlut();
	void initGlew();
	void initTexture();
	bool checkFramebufferStatus();

public:
	void setLines();
	void setBlendType(int blendType);
	void makeMorphImage(float t);
	void getRender(char* data);

	CRenderer(CMarkUI* imgA, CMarkUI* imgB);
	~CRenderer(void);
};

