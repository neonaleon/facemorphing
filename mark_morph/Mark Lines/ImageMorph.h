#pragma once

#include <cv.h>
#include <highgui.h>
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
