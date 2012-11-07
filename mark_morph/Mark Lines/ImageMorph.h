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
	bool m_isConsistent;

	//char* m_outputData;
	//IplImage* m_outputImage;
	//CvVideoWriter* vidw;

	// App states
	int m_width, m_height;
	/*int m_blendType;
	bool m_isPlaying;
	int m_playDirection;
	int m_frameNumber, m_frameTotal;
	int64 m_lastTime;
	bool m_isRendering;
	bool m_showDebugLines;*/

public:
	void run();
	void onLineUpdate();
	void forwardKeyPress(unsigned char key, int x, int y);
	void writeVideo();

	CImageMorph(void);
	~CImageMorph(void);

private:
	void initGlew();
	//static void onMousePress(int event, int x, int y, int flags, void* param);
	//bool onKeyPress(unsigned char key);
	//void onRedraw();
	//void drawLines(float t);
};
