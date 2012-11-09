/////////////////////////////////////////////////////////////////////////////
// File: MarkUI.h
// 
// Edit window class
// CMarkUI contains all states required for the editing windows
// This class is also responsible for keeping a copy of the original input
// image.
//
// Author: Daniel Seah
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>
#include <cv.h>
#include <highgui.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "IGLUTDelegate.h"

using namespace std;

class CImageMorph;

class CMarkUI : public IGLUTDelegate
{
private:
	struct Line2D
	{
		int start, end;
	};

private:
	vector<CvPoint2D32f> m_vertexBuffer;
	vector<Line2D> m_indexBuffer;
	vector<float> m_packedLine;

	char m_imgFilename[31];
	char m_lineFilename[31];

	IplImage* m_inImage;
	float m_imgScale;
	int m_imgWidth, m_imgHeight;
	GLuint m_image;

	bool m_isModified;
	int m_prevVertex;
	int m_dragPoint;
	bool m_isLeftMouseDown;

	CImageMorph *m_app;

public:
	float* getPackedLine();
	char* getImageData();
	IplImage* getImage();
	int getNumLines();

	void loadLines();
	void saveLines();

	CMarkUI(CImageMorph *app, const char* filename);
	~CMarkUI(void);

private:
	int searchPoint(float x, float y, float radius);
	void searchLines(int pointIndex, vector<int>* adjacentLines);

	virtual void onMousePress(int button, int state, int x, int y);
	virtual void onMouseMove(int x, int y);
	virtual void onResize(int width, int height);
	virtual void onRender();
	virtual void onKeyPress(unsigned char key, int x, int y);

	void initGLState();
	void initTexture();
};

