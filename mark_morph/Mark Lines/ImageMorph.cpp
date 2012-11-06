#include "ImageMorph.h"
#include "constants.h"
#include "MarkUI.h"
#include "Renderer.h"
#include <cv.h>

//extern const char *IMAGEA, *IMAGEB;

CImageMorph::CImageMorph(void)
{
	// Init application states
	m_blendType = 0;
	m_frameNumber = 0;
	m_frameTotal = FRAMERATE * DURATION;
	m_isPlaying = false;
	m_playDirection = 1;
	m_isRendering = false;
	m_isConsistent = false;
	m_showDebugLines = false;

	// Initialise 2 edit windows
	m_imageA = new CMarkUI(this, IMAGEA);
	cvNamedWindow(IMAGEA, CV_WINDOW_AUTOSIZE);
	cvSetMouseCallback(IMAGEA, CImageMorph::onMousePress, m_imageA);
	m_imageB = new CMarkUI(this, IMAGEB);
	cvNamedWindow(IMAGEB, CV_WINDOW_AUTOSIZE);
	cvSetMouseCallback(IMAGEB, CImageMorph::onMousePress, m_imageB);

	// Check if images are same size
	if(m_imageA->getImage()->width != m_imageB->getImage()->width ||
		m_imageA->getImage()->height != m_imageB->getImage()->height)
	{
		fprintf( stderr, "Error: Image size not identical\n");
		char ch; scanf( "%c", &ch ); // Prevents the console window from closing.
		exit( 1 );
	}

	// Initialise output window
	m_renderer = new CRenderer(m_imageA, m_imageB);
	m_renderer->setBlendType(m_blendType);
	cvNamedWindow("Morphed Image", CV_WINDOW_AUTOSIZE);

	// Create output image buffers
	CvSize size;
	size.width = m_imageA->getImage()->width;
	size.height = m_imageA->getImage()->height;
	m_outputData = new char[size.width * size.height * 3];
	m_outputImage = cvCreateImageHeader(size, IPL_DEPTH_8U, 3);	
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
	cvDestroyAllWindows();
	delete m_renderer;
	delete m_imageA;
	delete m_imageB;
}

void CImageMorph::onMousePress( int event, int x, int y, int flags, void* param )
{
	CMarkUI* receiver = (CMarkUI*) param;
	receiver->onMousePress(event, x, y, flags, NULL);
}

void CImageMorph::run()
{
	bool hasNext = true;
	while(hasNext)
	{
		hasNext = onKeyPress(cvWaitKey(33));
		cvShowImage(IMAGEA, m_imageA->getImage());
		cvShowImage(IMAGEB, m_imageB->getImage());
		if(m_isPlaying)
			onRedraw();
		cvShowImage("Morphed Image", m_outputImage);
	}
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
	onRedraw();
}

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
}

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
}

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
}

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
}