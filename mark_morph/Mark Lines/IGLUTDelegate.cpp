/////////////////////////////////////////////////////////////////////////////
// File: IGLUTDelegate.cpp
// 
// Delegate interface
// Classes implementing IGLUTDelgate will receive window events from GLUT.
// GL context will be set to the active GLUT window sending the event.
// All event handlers are optional.
//
// Author: Daniel Seah
/////////////////////////////////////////////////////////////////////////////

#include "IGLUTDelegate.h"
#include "GLUTWindow.h"


IGLUTDelegate::IGLUTDelegate(void)
{
}


IGLUTDelegate::~IGLUTDelegate(void)
{
}

void IGLUTDelegate::setWindow( CGLUTWindow *win )
{
	m_window = win;
	m_window->setDelegate(this);
}

void IGLUTDelegate::onMousePress( int button, int state, int x, int y )
{

}

void IGLUTDelegate::onMouseMove( int x, int y )
{

}

void IGLUTDelegate::onKeyPress( unsigned char key, int x, int y )
{

}

void IGLUTDelegate::onResize( int width, int height )
{

}

void IGLUTDelegate::onRender()
{

}

CGLUTWindow* IGLUTDelegate::getWindow()
{
	return m_window;
}

void IGLUTDelegate::onUpdate()
{

}
