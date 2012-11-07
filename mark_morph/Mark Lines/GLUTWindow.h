#pragma once
#include <map>
#include <GL/glew.h>
#include <GL/glut.h>
//#include "IGLUTDelegate.h"

using namespace std;

class IGLUTDelegate;

class CGLUTWindow
{
private:
	GLuint m_window;
	int m_winWidth, m_winHeight;

	static map<GLuint, IGLUTDelegate*> m_delegateList;

public:
	void setDelegate(IGLUTDelegate* delegate);
	int getWidth();
	int getHeight();
	int getWindow();

	static void onMousePress(int button, int state, int x, int y);
	static void onMouseMove(int x, int y);
	static void onKeyPress(unsigned char key, int x, int y);
	static void onResize(int width, int height);
	static void onRender();
	static void onUpdate();

	CGLUTWindow(const char* name, int width, int height);
	~CGLUTWindow(void);
};

