/////////////////////////////////////////////////////////////////////////////
// File: ImageMorph.h
// 
// Main application class
// This class contains all the program states, renderer and edit windows
//
// Author: Daniel Seah, Leon Ho
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

using namespace std;

class CMarkUI;
class CRenderer;
class CGLUTWindow;

class CImageMorph
{
private:
	CMarkUI *m_imageA, *m_imageB;
	CRenderer *m_renderer;
	vector<CGLUTWindow*> m_windowList;
	
	// App states
	int m_width, m_height;
	bool m_isConsistent;
	int m_outputLineCount;

public:
	void run();
	void onLineUpdate();
	void forwardKeyPress(unsigned char key, int x, int y);
	void writeVideo();

	CImageMorph(void);
	~CImageMorph(void);

private:
	void initGlew();
};
