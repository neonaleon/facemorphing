/////////////////////////////////////////////////////////////////////////////
// File: ImageMorph.cpp
// 
// Main application class
// This class contains all the program states, renderer and edit windows
//
// Author: Daniel Seah, Leon Ho
/////////////////////////////////////////////////////////////////////////////

#include "ImageMorph.h"
#include "constants.h"
#include "MarkUI.h"
#include "Renderer.h"
#include "GLUTWindow.h"
#include <cv.h>
#include <highgui.h>

using namespace cv;

CImageMorph::CImageMorph(void)
{
	// Init application states
	m_isConsistent = false;

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

	onLineUpdate();

	// Display user instructions in console window.
	printf( "Press and hold 'A/D' to control morphing\n" );
	printf( "Press 'R' to render to file\n" );
	printf( "Press 'T' to morph between faces\n" );
	printf( "Press 'X' to toggle between blending\n(Cross-dissolve, source only, destination only)\n" );
	printf( "Press 'L' to show lines\n" );
	printf( "Press '0-9' to go to positions\n" );
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

void CImageMorph::run()
{
	glutMainLoop();
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
	m_outputLineCount = m_imageA->getNumLines();
	m_renderer->setLines();
}

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
	printf("\nRendering to %s...\nDo not close window!\n", OUTVIDEO);
	printf("Width: %d\tHeight: %d\tFrames: %d\tLines: %d\n", m_width, m_height,
		FRAMERATE*DURATION+1, m_outputLineCount);

	int currentTime = glutGet(GLUT_ELAPSED_TIME);
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

	float elapsed = (glutGet(GLUT_ELAPSED_TIME) - currentTime) / 1000.0f;
	printf("Render complete\n");
	printf("Time taken: %.3f\n\n", elapsed);
}
