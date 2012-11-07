#include "MarkUI.h"
#include "ImageMorph.h"
#include "constants.h"
#include <GL/glew.h>
#include <GL/glut.h>
#include "shader_util.h"
#include "GLUTWindow.h"

//extern const CvScalar MARKCOLOR;
//extern const int CIRCLE_SIZE;

CMarkUI::CMarkUI(CImageMorph *app, const char* filename)
{
	m_app = app;
	m_isModified = true;
	m_prevVertex = -1;
	m_dragPoint = -1;
	m_imgScale = 1;

	strcpy(m_imgFilename, filename);
	m_inImage = cvLoadImage(m_imgFilename, CV_LOAD_IMAGE_UNCHANGED);
	cvFlip(m_inImage);
	m_imgWidth = m_inImage->width;
	m_imgHeight = m_inImage->height;

	string fn = filename;
	string lineFilename = fn.substr(0, fn.find_last_of(".")).append(".mld");
	strcpy(m_lineFilename, lineFilename.c_str());

	initGLState();
	initTexture();

	loadLines();
	//onRedraw();
}

CMarkUI::~CMarkUI(void)
{
	saveLines();
	cvReleaseImage(&m_inImage);
}

void CMarkUI::initGLState()
{
	// Set some OpenGL states and projection.
	glDisable( GL_DITHER );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );
	glDisable( GL_ALPHA_TEST );
	glDisable( GL_COLOR_LOGIC_OP );
	glDisable( GL_SCISSOR_TEST );
	glDisable( GL_STENCIL_TEST );
	glEnable(GL_TEXTURE_2D);
	glPolygonMode( GL_FRONT, GL_FILL );
}

//---------------------------------------------------------------------------
// Upload both face images and line information to GPU
//---------------------------------------------------------------------------
void CMarkUI::initTexture()
{
	// Create texture image
	glActiveTexture( GL_TEXTURE0 );
	glGenTextures( 1, &m_image );
	glBindTexture( GL_TEXTURE_2D, m_image );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB,
		m_inImage->width, m_inImage->height, 0, GL_BGR, GL_UNSIGNED_BYTE, m_inImage->imageData);
	printOpenGLError();
};

float* CMarkUI::getPackedLine()
{
	if(m_indexBuffer.size() <= 0)
		return NULL;
	if(!m_isModified)
		return &m_packedLine[0];

	// Pack lines for openGL upload
	m_packedLine.clear();
	for(auto it=m_indexBuffer.begin(); it!=m_indexBuffer.end(); it++)
	{
		m_packedLine.push_back(m_vertexBuffer[it->start].x);
		m_packedLine.push_back(m_vertexBuffer[it->start].y);
		m_packedLine.push_back(m_vertexBuffer[it->end].x);
		m_packedLine.push_back(m_vertexBuffer[it->end].y);
	}

	m_isModified = false;
	return &m_packedLine[0];
}

char* CMarkUI::getImageData()
{
	return m_inImage->imageData;
}

int CMarkUI::getNumLines()
{
	return m_indexBuffer.size();
}

void CMarkUI::loadLines()
{
	float ax, ay, bx, by;

	// Read image lines
	FILE* lineFile = fopen(m_lineFilename, "r");
	if(lineFile == NULL)
		return;
	while(fscanf(lineFile, "%f %f %f %f", &ax, &ay, &bx, &by) != EOF)
	{
		ay = m_inImage->height - ay;	// Inverts y dimension to match openGL format
		by = m_inImage->height - by;
		int ptAIndex = searchPoint(ax, ay, 1);
		if(ptAIndex <= -1)
		{
			CvPoint2D32f pt; pt.x = ax; pt.y = ay;
			m_vertexBuffer.push_back(pt);
			ptAIndex = m_vertexBuffer.size() - 1;
		}
		int ptBIndex = searchPoint(bx, by, 1);
		if(ptBIndex <= -1)
		{
			CvPoint2D32f pt; pt.x = bx; pt.y = by;
			m_vertexBuffer.push_back(pt);
			ptBIndex = m_vertexBuffer.size() - 1;
		}
		Line2D line; line.start = ptAIndex; line.end = ptBIndex;
		m_indexBuffer.push_back(line);
	}
	fclose(lineFile);
}

void CMarkUI::saveLines()
{
	float ax, ay, bx, by;
	FILE* lineFile = fopen(m_lineFilename, "w");
	for(auto it=m_indexBuffer.begin(); it!=m_indexBuffer.end(); it++)
	{
		ax = m_vertexBuffer[it->start].x;
		ay = m_inImage->height - m_vertexBuffer[it->start].y;
		bx = m_vertexBuffer[it->end].x;
		by = m_inImage->height - m_vertexBuffer[it->end].y;
		fprintf(lineFile, "%f %f %f %f\n", ax, ay, bx, by);
	}
	fclose(lineFile);
}

void CMarkUI::onMousePress( int button, int state, int x, int y )
{
	int currPtIndex;
	vector<int> adjacentLines;

	// Convert point to image space
	int winWidth = m_window->getWidth();
	int winHeight = m_window->getHeight();
	int leftBorder = (m_window->getWidth() - m_imgScale * m_imgWidth) / 2.0f;
	int bottomBorder = (m_window->getHeight() - m_imgScale * m_imgHeight) / 2.0f;
	x = (x - leftBorder) / m_imgScale;
	y = (y - bottomBorder) / m_imgScale;
	y = m_imgHeight - y;

	// Check if click is within image
	if(x < 0 || x >= m_imgWidth || y < 0 || y >= m_imgHeight)
		return;

	if(button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		m_isLeftMouseDown = false;

		// User has stopped dragging
		m_dragPoint = -1;

		// Add new point if it is sufficiently far
		currPtIndex = searchPoint(x, y, 5);
		if(currPtIndex == -1)
		{
			CvPoint2D32f pt; pt.x = x; pt.y = y;
			m_vertexBuffer.push_back(pt);
			currPtIndex = m_vertexBuffer.size() - 1;
		}

		// Add line segment
		if(currPtIndex == m_prevVertex)
			return;
		if(m_prevVertex != -1)
		{
			Line2D line; line.start = m_prevVertex, line.end = currPtIndex;
			m_indexBuffer.push_back(line);
		}
		m_prevVertex = currPtIndex;

		// Inform app of line change
		m_isModified = true;
		m_app->onLineUpdate();
		glutSetWindow(m_window->getWindow());
		glutPostRedisplay();
	} else if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		m_isLeftMouseDown = true;
		m_dragPoint = searchPoint(x, y, 5);
	} else if(button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
	{
		// End line segment
		if(m_prevVertex != -1)
		{
			m_prevVertex = -1;
			return;
		}

		// Search for point to delete
		currPtIndex = searchPoint(x, y, 5);
		if(currPtIndex == -1)
			return;

		// Remove adjacent lines
		searchLines(currPtIndex, &adjacentLines);
		for(auto it=adjacentLines.rbegin(); it!=adjacentLines.rend(); it++)
		{
			m_indexBuffer.erase(m_indexBuffer.begin() + *it);
		}

		// Inform app of line change
		m_isModified = true;
		m_app->onLineUpdate();
		glutSetWindow(m_window->getWindow());
		glutPostRedisplay();
	}
	
}

int CMarkUI::searchPoint( float x, float y, float radius )
{
	float minx = x - radius;
	float maxx = x + radius;
	float miny = y - radius;
	float maxy = y + radius;

	for(auto it=m_indexBuffer.begin(); it!=m_indexBuffer.end(); it++)
	{
		CvPoint2D32f start = m_vertexBuffer[it->start];
		CvPoint2D32f end = m_vertexBuffer[it->end];
		if(start.x >= minx && start.x <= maxx && start.y >= miny && start.y <= maxy)
			return it->start;
		if(end.x >= minx && end.x <= maxx && end.y >= miny && end.y <= maxy)
			return it->end;
	}
	return -1;
}

void CMarkUI::searchLines( int pointIndex, vector<int>* adjacentLines )
{
	for(int i=0; i<m_indexBuffer.size(); i++)
	{
		if(m_indexBuffer[i].start == pointIndex || m_indexBuffer[i].end == pointIndex)
			adjacentLines->push_back(i);
		if(adjacentLines->size() >= 2)
			return;
	}
	return;
}

IplImage* CMarkUI::getImage()
{
	return m_inImage;
}

void CMarkUI::onMouseMove( int x, int y )
{
	if(!m_isLeftMouseDown || m_dragPoint == -1)
		return;

	// End line segment
	m_prevVertex = -1;

	m_vertexBuffer[m_dragPoint].x = x;
	m_vertexBuffer[m_dragPoint].y = y;

	// Inform app of line change
	m_isModified = true;
	//m_app->onLineUpdate();
	glutPostRedisplay();
}

void CMarkUI::onResize( int width, int height )
{
	int winWidth = m_window->getWidth();
	int winHeight = m_window->getHeight();

	// Determine image scale
	m_imgScale = (float)winWidth / m_imgWidth;
	if(winHeight < m_imgHeight * m_imgScale)
		m_imgScale = (float)winHeight / m_imgHeight;

	//glutPostRedisplay();
}

void CMarkUI::onRender()
{
	int winWidth = m_window->getWidth();
	int winHeight = m_window->getHeight();
	int leftBorder = (m_window->getWidth() - m_imgScale * m_imgWidth) / 2.0f;
	int bottomBorder = (m_window->getHeight() - m_imgScale * m_imgHeight) / 2.0f;

	// Reset projection, modelview matrices and viewport.
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluOrtho2D( 0, winWidth, 0, winHeight );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glViewport( 0, 0, winWidth, winHeight );

	// Reset GL states
	glClearColor(0.243, 0.243, 0.243, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw image
	printOpenGLError();
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, m_image);
	glBegin( GL_QUADS );
	glTexCoord2f(0, 0);
	glVertex2f(leftBorder, bottomBorder);
	glTexCoord2f(0, 1);
	glVertex2f(leftBorder, bottomBorder + m_imgHeight * m_imgScale);
	glTexCoord2f(1, 1);
	glVertex2f(leftBorder + m_imgWidth * m_imgScale, bottomBorder + m_imgHeight * m_imgScale);
	glTexCoord2f(1, 0);
	glVertex2f(leftBorder + m_imgWidth * m_imgScale, bottomBorder);
	glEnd();
	
	// Draw points
	glDisable(GL_TEXTURE_2D);
	glColor3fv(MARKCOLOR);
	glPointSize(CIRCLE_SIZE);
	glBegin(GL_POINTS);
	if(m_prevVertex != -1)
	{
		CvPoint2D32f first = m_vertexBuffer[m_prevVertex];
		glVertex2f(leftBorder+first.x*m_imgScale, bottomBorder+first.y*m_imgScale);
		//first.y = m_inImage->height - first.y;
		//cvCircle(m_frameBuffer, first, CIRCLE_SIZE, MARKCOLOR);
	}
	for(int i=0; i<m_indexBuffer.size(); i++)
	{
		CvPoint2D32f start = m_vertexBuffer[m_indexBuffer[i].start];
		CvPoint2D32f end = m_vertexBuffer[m_indexBuffer[i].end];
		glVertex2f(leftBorder+start.x*m_imgScale, bottomBorder+start.y*m_imgScale);
		glVertex2f(leftBorder+end.x*m_imgScale, bottomBorder+end.y*m_imgScale);
	}
	glEnd();

	// Draw lines
	glBegin(GL_LINES);
	for(int i=0; i<m_indexBuffer.size(); i++)
	{
		CvPoint2D32f start = m_vertexBuffer[m_indexBuffer[i].start];
		CvPoint2D32f end = m_vertexBuffer[m_indexBuffer[i].end];
		glVertex2f(leftBorder+start.x*m_imgScale, bottomBorder+start.y*m_imgScale);
		glVertex2f(leftBorder+end.x*m_imgScale, bottomBorder+end.y*m_imgScale);

		/*char number[31];
		itoa(i+1, number, 10);
		CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 0.8, 0.8);
		cvPutText(m_frameBuffer, number, start, &font, MARKCOLOR);*/
	}
	glEnd();
	
	glutSwapBuffers();
}

void CMarkUI::onKeyPress( unsigned char key, int x, int y )
{
	m_app->forwardKeyPress(key, x, y);
}
