#pragma once

class CGLUTWindow;

class IGLUTDelegate
{
protected:
	CGLUTWindow *m_window;

public:
	void setWindow(CGLUTWindow *win);
	CGLUTWindow* getWindow();

	IGLUTDelegate(void);
	virtual ~IGLUTDelegate(void);

	virtual void onMousePress(int button, int state, int x, int y);
	virtual void onMouseMove(int x, int y);
	virtual void onKeyPress(unsigned char key, int x, int y);
	virtual void onResize(int width, int height);
	virtual void onRender();
	virtual void onUpdate();
};

