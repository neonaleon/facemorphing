/////////////////////////////////////////////////////////////////////////////
// File: GLUTWindow.cpp
// 
// Wrapper class for a GLUT Window
// CGLUTWindow manages a GLUT window and its c-style event callbacks.
// GLUT event callbacks are forwarded to a delegate class. GL context is
// also guranteed to belong to the active GLUT window.
//
// Author: Daniel Seah
/////////////////////////////////////////////////////////////////////////////

#include "GLUTWindow.h"
#include "IGLUTDelegate.h"

using namespace std;

map<GLuint, IGLUTDelegate*> CGLUTWindow::m_delegateList;

CGLUTWindow::CGLUTWindow(const char* name, int width, int height)
{
	m_winWidth = width;
	m_winHeight = height;

	// Create window
	glutInitWindowSize(m_winWidth, m_winHeight);
	m_window = glutCreateWindow(name);

	// Bind callbacks
	glutDisplayFunc(onRender);
	glutReshapeFunc(onResize);
	glutKeyboardFunc(onKeyPress);
	glutMotionFunc(onMouseMove);
	glutMouseFunc(onMousePress);
	glutIdleFunc(onUpdate);
}

CGLUTWindow::~CGLUTWindow(void)
{
	glutDestroyWindow(m_window);
}

void CGLUTWindow::setDelegate( IGLUTDelegate* delegate )
{
	if(delegate)
		m_delegateList[m_window] = delegate;
	else
		m_delegateList[m_window] = NULL;
}

void CGLUTWindow::onMousePress( int button, int state, int x, int y )
{
	m_delegateList[glutGetWindow()]->onMousePress(button, state, x, y);
}

void CGLUTWindow::onMouseMove( int x, int y )
{
	m_delegateList[glutGetWindow()]->onMouseMove(x, y);
}

void CGLUTWindow::onKeyPress( unsigned char key, int x, int y )
{
	m_delegateList[glutGetWindow()]->onKeyPress(key, x, y);
}

void CGLUTWindow::onResize( int width, int height )
{
	m_delegateList[glutGetWindow()]->getWindow()->m_winWidth = width;
	m_delegateList[glutGetWindow()]->getWindow()->m_winHeight = height;
	m_delegateList[glutGetWindow()]->onResize(width, height);
}

void CGLUTWindow::onUpdate()
{
	for(auto it=m_delegateList.begin(); it!=m_delegateList.end(); it++)
	{
		glutSetWindow(it->first);
		it->second->onUpdate();
	}
}

void CGLUTWindow::onRender()
{
	m_delegateList[glutGetWindow()]->onRender();
}

int CGLUTWindow::getWidth()
{
	return m_winWidth;
}

int CGLUTWindow::getHeight()
{
	return m_winHeight;
}

int CGLUTWindow::getWindow()
{
	return m_window;
}
