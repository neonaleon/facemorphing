/////////////////////////////////////////////////////////////////////////////
// File: Renderer.h
// 
// Output renderer video
// CMarkUI contains all states required for output video window.
// This class also hooks up with GLSL and performs rendering of the warped
// image. 
//
// Author: Daniel Seah
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "IGLUTDelegate.h"

class CMarkUI;
class CImageMorph;

class CRenderer : public IGLUTDelegate
{
private:
	CImageMorph *m_app;
	CMarkUI *m_pImageA, *m_pImageB;
	int m_imgWidth, m_imgHeight;
	float m_imgScale;

	GLuint m_morphProg;
	GLuint m_texA, m_texB, m_texLineA, m_texLineB, m_morphedTexObj;
	GLuint m_fbo;

	int m_lastTime;
	int m_frameNumber, m_frameTotal;
	bool m_isPlaying;
	float m_playDirection;
	int m_blendType;
	bool m_showDebugLines;

private:
	void initGLState();
	void initShader();
	void initTexture();
	void drawLines(float t);
	void drawMorphImage();
	bool checkFramebufferStatus();

public:
	void setLines();
	void setBlendType(int blendType);
	void makeMorphImage(float t);
	void getRender(char* data);

	CRenderer(CImageMorph *app, CMarkUI* imgA, CMarkUI* imgB);
	~CRenderer(void);

public:
	virtual void onKeyPress(unsigned char key, int x, int y);
	virtual void onResize(int width, int height);
	virtual void onRender();
	virtual void onUpdate();
};

