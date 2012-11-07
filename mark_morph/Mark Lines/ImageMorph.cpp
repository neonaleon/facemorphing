#include "ImageMorph.h"
#include "constants.h"
#include "MarkUI.h"
#include "Renderer.h"
#include <GL/glew.h>
#include <GL/glut.h>
#include <cv.h>
#include "GLUTWindow.h"

//extern const char *IMAGEA, *IMAGEB;

CImageMorph::CImageMorph(void)
{
	// Init application states
	/*m_blendType = 0;
	m_frameNumber = 0;
	m_frameTotal = FRAMERATE * DURATION;
	m_isPlaying = false;
	m_playDirection = 1;
	m_isRendering = false;
	m_isConsistent = false;
	m_showDebugLines = false;*/

	// Read image size
	IplImage *imga = cvLoadImage(IMAGEA);
	IplImage *imgb = cvLoadImage(IMAGEB);
	m_width = imga->width;
	m_height = imga->height;
	int widthB = imgb->width;
	int heightB = imgb->height;
	cvReleaseImage(&imga);
	cvReleaseImage(&imgb);

	// Check if images are same size
	if(m_width != widthB || m_height != heightB)
	{
		fprintf( stderr, "Error: Image size not identical\n");
		char ch; scanf( "%c", &ch ); // Prevents the console window from closing.
		exit( 1 );
	}

	// Initialize GLUT.
	int argc = 1;
	char *argv[] = {"CS4243 Image Morph", NULL};
	glutInit( &argc, argv );
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);

	// Initialise 2 edit windows
	CGLUTWindow *win = new CGLUTWindow("Image A", m_width, m_height);
	initGlew();		// Initialise GLEW and check for compatibility
	m_imageA = new CMarkUI(this, IMAGEA);
	m_imageA->setWindow(win);
	m_windowList.push_back(win);
	win = new CGLUTWindow("Image B", m_width, m_height);
	m_imageB = new CMarkUI(this, IMAGEB);
	m_imageB->setWindow(win);
	m_windowList.push_back(win);

	// Initialise output window
	win = new CGLUTWindow("Morphed Image", m_width, m_height);
	m_renderer = new CRenderer(this, m_imageA, m_imageB);
	m_renderer->setWindow(win);
	m_windowList.push_back(win);

	

	// Create output image buffers
	/*CvSize size;
	size.width = m_width;
	size.height = m_height;
	m_outputData = new char[m_width * m_height * 3];
	m_outputImage = cvCreateImageHeader(size, IPL_DEPTH_8U, 3);	*/
	onLineUpdate();

	// Display user instructions in console window.
	printf( "Press and hold 'A/D' to control morphing\n" );
	printf( "Press 'R' to render to file\n" );
	printf( "Press 'T' to morph between faces\n" );
	printf( "Press 'X' to toggle between blending\n(Cross-dissolve, source only, destination only)\n" );
	printf( "Press 'L' to show lines\n" );
	printf( "Press 'Q' to quit.\n\n" );
}

CImageMorph::~CImageMorph(void)
{
	for(auto it=m_windowList.begin(); it!=m_windowList.end(); it++)
		delete (*it);
	m_windowList.clear();
	delete m_renderer;
	delete m_imageA;
	delete m_imageB;
}
/*
void CImageMorph::onMousePress( int event, int x, int y, int flags, void* param )
{
	CMarkUI* receiver = (CMarkUI*) param;
	receiver->onMousePress(event, x, y, flags, NULL);
}*/

void CImageMorph::run()
{
	glutMainLoop();
	/*
	bool hasNext = true;
	while(hasNext)
	{
		hasNext = onKeyPress(cvWaitKey(33));
		cvShowImage(IMAGEA, m_imageA->getImage());
		cvShowImage(IMAGEB, m_imageB->getImage());
		if(m_isPlaying)
			onRedraw();
		cvShowImage("Morphed Image", m_outputImage);
	}*/
}

void CImageMorph::onLineUpdate()
{
	// Check if we have the same number of lines on both images
	if(m_imageA->getNumLines() != m_imageB->getNumLines())
	{
		m_isConsistent = false;
		return;
	}

	m_isConsistent = true;
	m_renderer->setLines();
}
/*
bool CImageMorph::onKeyPress(unsigned char key)
{
	if(m_isRendering)
		return true;

	switch ( key )
	{
	case 'r':
	case 'R':
		m_isRendering = true;
		if(!m_isConsistent)
			printf("Warning: No corresponding pairs of lines between both images.\n");
		printf("Rendering to %s...\nDo not close window!\n", OUTVIDEO);
		writeVideo();
		m_isRendering = false;
		printf("Render complete\n");
		break;

	case 't':
	case 'T':
		m_isPlaying = !m_isPlaying;
		m_lastTime = cvGetTickCount();
		if(m_frameNumber <= 0)
			m_playDirection = 1;
		else if(m_frameNumber >= m_frameTotal)
			m_playDirection = -1;
		break;

	case 'x':
	case 'X':
		m_blendType = ++m_blendType % 3;
		if(m_blendType == 0)
			printf("Blend Mode: Cross-dissolve\n");
		else if(m_blendType == 1)
			printf("Blend Mode: Source only\n");
		else
			printf("Blend Mode: Destination only\n");
		m_renderer->setBlendType(m_blendType);
		onRedraw();
		break;

	case 'a':
	case 'A':
		m_frameNumber--;
		if(m_frameNumber < 0)
			m_frameNumber = 0;
		onRedraw();
		cvWaitKey(0);
		break;

	case 'd':
	case 'D':
		m_frameNumber++;
		if(m_frameNumber > m_frameTotal)
			m_frameNumber = m_frameTotal;
		onRedraw();
		cvWaitKey(0);
		break;

	case 'l':
	case 'L':
		m_showDebugLines = !m_showDebugLines;
		onRedraw();
		break;

	// Quit program.
	case 'q':
	case 'Q': 
	case 27:
		return false;
		break;
	}

	return true;
}*/

/*
void CImageMorph::onRedraw()
{
	// Calculate frame position and thus t
	int64 currentTime = cvGetTickCount();
	int64 elapsed = currentTime - m_lastTime;
	m_lastTime = currentTime;
	if(m_isPlaying)
	{
		m_frameNumber += elapsed / cvGetTickFrequency() / 1000000 * m_playDirection *FRAMERATE;
		if(m_frameNumber < 0)
		{
			m_frameNumber = 0;
			m_isPlaying = false;
		}
		if(m_frameNumber > m_frameTotal)
		{
			m_frameNumber = m_frameTotal;
			m_isPlaying = false;
		}
	}
	float t = float(m_frameNumber) / m_frameTotal;

	m_renderer->makeMorphImage(t);
	m_renderer->getRender(m_outputData);

	m_outputImage->imageData = m_outputData;
	m_outputImage->imageDataOrigin = m_outputImage->imageData;
	cvFlip(m_outputImage, 0);

	if(m_showDebugLines)
		drawLines(t);
}*/
/*
void CImageMorph::writeVideo()
{
	m_isPlaying = !m_isPlaying;
	m_showDebugLines = false;

	vidw = cvCreateVideoWriter(OUTVIDEO, CODEC, FRAMERATE,
		Size(m_outputImage->width, m_outputImage->height));

	for(int i=0; i<=m_frameTotal; i++)
	{
		m_renderer->makeMorphImage(float(i) / m_frameTotal);
		m_renderer->getRender(m_outputData);
		m_outputImage->imageData = m_outputData;
		m_outputImage->imageDataOrigin = m_outputImage->imageData;
		cvFlip(m_outputImage, 0);
		cvWriteFrame(vidw, m_outputImage);
	}
	cvReleaseVideoWriter(&vidw);
}*/

/*
void CImageMorph::drawLines(float t)
{
	float* lineA = m_imageA->getPackedLine();
	float* lineB = m_imageB->getPackedLine();
	int numLines = m_imageA->getNumLines();
	int height = m_imageA->getImage()->height;
	CvPoint2D32f start, end;

	for(int i=0; i<numLines; i++)
	{
		start.x = lineA[i*4] * (1-t) + lineB[i*4] * t;
		start.y = lineA[i*4+1] * (1-t) + lineB[i*4+1] * t;
		end.x = lineA[i*4+2] * (1-t) + lineB[i*4+2] * t;
		end.y = lineA[i*4+3] * (1-t) + lineB[i*4+3] * t;
		start.y = height - start.y;
		end.y = height - end.y;
		cvLine(m_outputImage, cvPointFrom32f(start), cvPointFrom32f(end), MARKCOLOR);
	}
}*/

//---------------------------------------------------------------------------
// Check for OpenGL 2.0 and the necessary OpenGL extensions
//---------------------------------------------------------------------------
void CImageMorph::initGlew()
{
	// Initialize GLEW.
	GLenum err = glewInit();
	if ( err != GLEW_OK )
	{
		fprintf( stderr, "Error: %s.\n", glewGetErrorString( err ) );
		char ch; scanf( "%c", &ch ); // Prevents the console window from closing.
		exit( 1 );
	}

	// Make sure OpenGL 2.0 is supported.
	if ( !GLEW_VERSION_2_0 )
	{
		fprintf( stderr, "Error: OpenGL 2.0 is not supported.\n" );
		char ch; scanf( "%c", &ch ); // Prevents the console window from closing.
		exit( 1 );
	}

	// Make sure necessary OpenGL extensions are supported.
	bool extSupported = true;
	if ( !GLEW_ARB_texture_float)
	{
		fprintf( stderr, "Error: Float textures not supported.\n" );
		extSupported = false;
	}
	if ( !GLEW_ARB_texture_rectangle)
	{
		fprintf( stderr, "Error: Texture rectangles not supported.\n" );
		extSupported = false;
	}
	if ( !GLEW_ARB_framebuffer_object)
	{
		fprintf( stderr, "Error: Framebuffer objects not supported.\n" );
		extSupported = false;
	}
	if ( !extSupported)
	{
		char ch; scanf( "%c", &ch ); // Prevents the console window from closing.
		exit( 1 );
	}
}

void CImageMorph::forwardKeyPress( unsigned char key, int x, int y )
{
	m_renderer->onKeyPress(key, x, y);
}

void CImageMorph::writeVideo()
{
	CvSize size = Size(m_width, m_height);
	CvVideoWriter *vidw = cvCreateVideoWriter(OUTVIDEO, CODEC, FRAMERATE, size);

	char *outputData = new char[m_width * m_height * 3];
	IplImage *outputImage = cvCreateImageHeader(size, IPL_DEPTH_8U, 3);

	for(int i=0; i<=FRAMERATE*DURATION; i++)
	{
		m_renderer->makeMorphImage((float)i / (FRAMERATE*DURATION));
		m_renderer->getRender(outputData);
		outputImage->imageData = outputData;
		outputImage->imageDataOrigin = outputImage->imageData;
		cvFlip(outputImage, 0);
		cvWriteFrame(vidw, outputImage);
	}
	cvReleaseVideoWriter(&vidw);
}
