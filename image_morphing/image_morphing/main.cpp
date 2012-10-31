/////////////////////////////////////////////////////////////////////////////
// Image morphing program
//
// Author: Daniel Seah, Leon Ho
/////////////////////////////////////////////////////////////////////////////

// Program paramters
const char* imgAFilename = "inputa.jpg";
const char* imgBFilename = "inputb.jpg";
const char* lineAFilename = "inputa.mld";
const char* lineBFilename = "inputb.mld";
const char* output_video_filename = "outvid.avi";
const int frameRate = 24;
const int duration = 3;

// Shaders' filenames.
static const char vertShaderFilename[] = "morph.vert";
static const char fragShaderFilename[] = "morph.frag";

// Includes
#include <stdio.h>
#include <cv.h>
#include <highgui.h>
#include <vector>
#include <GL/glew.h>
#include <GL/glut.h>
#include "shader_util.h"

using namespace cv;
using namespace std;

// Shader program object.
static GLuint morphProg;

// GLUT window handle.
static GLuint glutWindowHandle;

// Image properties
int imgWidth, imgHeight;
int winWidth = 1024;
int winHeight = 768;
float imgScale;

// Texture image handle
GLuint texA, texB, texLineA, texLineB, morphedTexObj;
GLuint fbo;

// Line lists
int numLines;
float* lineA;
float* lineB;
float* lineInterp;

// Program states
int blendType;
bool showDebugLines;
bool isPlaying;
int playDirection;
int frameNumber, frameTotal;
int lastTime;

// Video Writer stuff
const int codec = 0;
CvVideoWriter* vidw = NULL;
bool isRendering;

// Forward declarations
static bool CheckFramebufferStatus();
static void onRender();
static void onUpdate(int value);
static void interpolateLines(float* src, float* dest, float* out, int size, float t);
static void MakeMorphImage(float t);
static void drawLines(float t);
static void onResize(int w, int h);
static void drawMorphImage();
static void writeVideo();

//---------------------------------------------------------------------------
// Keyboard callback function
//---------------------------------------------------------------------------
void onKeyPress( unsigned char key, int x, int y )
{
	if(isRendering)
		return;

	switch ( key )
	{
	case 'r':
	case 'R':
		isRendering = true;
		isPlaying = !isPlaying;
		printf("Rendering to %s...\nDo not close window!\n", output_video_filename);
		writeVideo();
		isRendering = false;
		printf("Render complete\n");
		break;

	case 't':
	case 'T':
		isPlaying = !isPlaying;
		lastTime =  glutGet(GLUT_ELAPSED_TIME);
		if(frameNumber <= 0)
			playDirection = 1;
		else if(frameNumber >= frameTotal)
			playDirection = -1;
		glutTimerFunc(1000.0f / frameRate, onUpdate, 0);
		break;

	case 'x':
	case 'X':
		blendType = ++blendType % 3;
		if(blendType == 0)
			printf("Blend Mode: Cross-dissolve\n");
		else if(blendType == 1)
			printf("Blend Mode: Source only\n");
		else
			printf("Blend Mode: Destination only\n");
		glutPostRedisplay();
		break;

	case 'a':
	case 'A':
		frameNumber--;
		if(frameNumber < 0)
			frameNumber = 0;
		glutPostRedisplay();
		break;

	case 'd':
	case 'D':
		frameNumber++;
		if(frameNumber > frameTotal)
			frameNumber = frameTotal;
		glutPostRedisplay();
		break;

	case 'l':
	case 'L':
		showDebugLines = !showDebugLines;
		glutPostRedisplay();
		break;

		// Quit program.
	case 'q':
	case 'Q': 
	case 27:
		glutDestroyWindow(glutWindowHandle);
		exit(0);
		break;
	}
}

void writeTexToVideo()
{
	CvSize size;
	size.height = imgHeight;
	size.width = imgWidth;

	char* imageData = new char[imgWidth * imgHeight * 3];

	glFinish();
	glBindTexture(GL_TEXTURE_2D, morphedTexObj);
	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, imageData);
	
	IplImage* renderedImage = cvCreateImageHeader(size, IPL_DEPTH_8U, 3);
	renderedImage->imageData = imageData;
	renderedImage->imageDataOrigin = renderedImage->imageData;
	cvFlip(renderedImage, 0);
	cvWriteFrame(vidw, renderedImage);
}

static void writeVideo()
{
	vidw = cvCreateVideoWriter(output_video_filename, codec, frameRate, Size(imgWidth, imgHeight));

	for(int i=0; i<=frameTotal; i++)
	{
		MakeMorphImage(float(i) / frameTotal);
		writeTexToVideo();
	}
	cvReleaseVideoWriter(&vidw);
}

//---------------------------------------------------------------------------
// Update loop
// This runs only when the video is playing
//---------------------------------------------------------------------------
static void onUpdate(int value)
{
	if(!isPlaying)
		return;

	glutTimerFunc(1000.0f / frameRate, onUpdate, value);
	onRender();
}

//---------------------------------------------------------------------------
// Render frame
//---------------------------------------------------------------------------
static void onRender()
{
	// Calculate frame position and thus t
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	int elapsed = currentTime - lastTime;
	if(isPlaying)
	{
		frameNumber += elapsed / 1000.0f * playDirection *frameRate;
		if(frameNumber < 0)
		{
			frameNumber = 0;
			isPlaying = false;
		}
		if(frameNumber > frameTotal)
		{
			frameNumber = frameTotal;
			isPlaying = false;
		}
	}
	float t = float(frameNumber) / frameTotal;

	MakeMorphImage(t);

	// Reset projection, modelview matrices and viewport.
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluOrtho2D( 0, winWidth, 0, winHeight );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glViewport( 0, 0, winWidth, winHeight );

	// Reset GL states
	glUseProgram(NULL);
	glClearColor(0.243, 0.243, 0.243, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);

	// Render morphed image
	drawMorphImage();

	// Render debug lines
	if(showDebugLines)
		drawLines(t);

	glutSwapBuffers();
	lastTime = currentTime;
}

//---------------------------------------------------------------------------
// Upload both face images and line information to GPU
//---------------------------------------------------------------------------
static void InitTexture( IplImage* imgA, IplImage* imgB )
{
	if(imgA->width != imgB->width || imgB->height != imgB->height)
	{
		fprintf( stderr, "Error: Image size not identical\n");
		char ch; scanf( "%c", &ch ); // Prevents the console window from closing.
		exit( 1 );
	}

	// Create texture A.
	glActiveTexture( GL_TEXTURE0 );
	glGenTextures( 1, &texA );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, texA );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB,
		imgWidth, imgHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, imgA->imageData );
	printOpenGLError();

	// Create texture B.
	glActiveTexture( GL_TEXTURE1 );
	glGenTextures( 1, &texB );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, texB );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB,
		imgWidth, imgHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, imgB->imageData );
	printOpenGLError();

	// Set image parameters in shader
	GLint uniTexA = glGetUniformLocation( morphProg, "TexA" );
	glUniform1i( uniTexA, 0 );
	GLint uniTexB = glGetUniformLocation( morphProg, "TexB" );
	glUniform1i( uniTexB, 1 );
	GLint uniTexWidthLoc = glGetUniformLocation( morphProg, "TexWidth" );
	glUniform1f( uniTexWidthLoc, (float)imgWidth );
	GLint uniTexHeightLoc = glGetUniformLocation( morphProg, "TexHeight" );
	glUniform1f( uniTexHeightLoc, (float)imgHeight );

	// Upload line A to GPU
	glActiveTexture( GL_TEXTURE2 );
	glGenTextures( 1, &texLineA );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, texLineA );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB,
		numLines, 1, 0, GL_RGBA, GL_FLOAT, lineA );
	printOpenGLError();

	// Upload line B to GPU
	glActiveTexture( GL_TEXTURE3 );
	glGenTextures( 1, &texLineB );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, texLineB );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB,
		numLines, 1, 0, GL_RGBA, GL_FLOAT, lineB );
	printOpenGLError();

	// Set line parameters in shader
	GLint uniLineA = glGetUniformLocation( morphProg, "ALines" );
	glUniform1i( uniLineA, 2 );
	GLint uniLineB = glGetUniformLocation( morphProg, "BLines" );
	glUniform1i( uniLineB, 3 );
	GLint uniLineCount = glGetUniformLocation( morphProg, "LineCount" );
	glUniform1f( uniLineCount, (float)numLines );

	// Create image output texture
	glActiveTexture( GL_TEXTURE4 );
	glGenTextures( 1, &morphedTexObj );
	glBindTexture( GL_TEXTURE_2D, morphedTexObj );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8,
		imgWidth, imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL );
	printOpenGLError();

	//-----------------------------------------------------------------------------
	// Attach the output texture to a FBO
	//-----------------------------------------------------------------------------
	glGenFramebuffersEXT( 1, &fbo ); 
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, fbo );
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
		GL_TEXTURE_2D, morphedTexObj, 0 );
	CheckFramebufferStatus();
	printOpenGLError();
};

//---------------------------------------------------------------------------
// Initialise GLUT window
// Set up persistent openGL rendering states
//---------------------------------------------------------------------------
static void InitGlut(int argc, char** argv)
{
	// Initialize GLUT.
	glutInit( &argc, argv );
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize (winWidth, winHeight);

	// This creates a OpenGL rendering context so that
	// we can start to issue OpenGL commands after this.
	glutWindowHandle = glutCreateWindow( "Image Morphing" );  

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

void onResize (int w, int h)
{
	winWidth = w;
	winHeight = h;

	// Determine image scale
	imgScale = (float)winWidth / imgWidth;
	if(winHeight < imgHeight * imgScale)
		imgScale = (float)winHeight / imgHeight;

	glutPostRedisplay();
}

//---------------------------------------------------------------------------
// Create a OpenGL rendering context. 
// Check for OpenGL 2.0 and the necessary OpenGL extensions. 
// Read in the shaders from files to create a shader program object.
//---------------------------------------------------------------------------
static void InitGlew()
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

	// Create shader program object.
	morphProg = makeShaderProgramFromFiles( vertShaderFilename, fragShaderFilename, NULL );
	if ( morphProg == 0 )
	{
		fprintf( stderr, "Error: Cannot create shader program object.\n" );
		char ch; scanf( "%c", &ch ); // Prevents the console window from closing.
		exit( 1 );
	}

	// Deploy user-defined shaders.
	glUseProgram( morphProg );
}

//---------------------------------------------------------------------------
// Read lines from mld file
//---------------------------------------------------------------------------
static void InitLine()
{
	float ax, ay, bx, by;
	vector<float> linesA, linesB;

	// Read image A lines
	FILE* lineAFile = fopen(lineAFilename, "r");
	while(fscanf(lineAFile, "%f %f %f %f", &ax, &ay, &bx, &by) != EOF)
	{
		ay = imgHeight - ay;	// Inverts y dimension to match openGL format
		by = imgHeight - by;
		linesA.push_back(ax);
		linesA.push_back(ay);
		linesA.push_back(bx);
		linesA.push_back(by);
	}
	lineA = new float[linesA.size()];
	for(int i=0; i<linesA.size(); i++)
		lineA[i] = linesA[i];
	fclose(lineAFile);

	// Read image B lines
	FILE* lineBFile = fopen(lineBFilename, "r");
	while(fscanf(lineBFile, "%f %f %f %f", &ax, &ay, &bx, &by) != EOF)
	{
		ay = imgHeight - ay;	// Inverts y dimension to match openGL format
		by = imgHeight - by;
		linesB.push_back(ax);
		linesB.push_back(ay);
		linesB.push_back(bx);
		linesB.push_back(by);
	}
	lineB = new float[linesB.size()];
	for(int i=0; i<linesB.size(); i++)
		lineB[i] = linesB[i];
	fclose(lineBFile);

	// Check if same number of lines in both file
	if(linesA.size() != linesB.size())
	{
		fprintf( stderr, "Error: Line count not equal\n");
		char ch; scanf( "%c", &ch ); // Prevents the console window from closing.
		exit( 1 );
	}
	numLines = linesB.size() / 4;

	// Create buffer for interpolated lines
	lineInterp = new float[numLines * 4];
}

int main(int argc, char* argv[])
{
	// Init global variables
	blendType = 0;
	frameNumber = 0;
	frameTotal = frameRate * duration;
	isPlaying = false;
	playDirection = 1;
	showDebugLines = false;
	isRendering = false;

	// Load images
	IplImage* imgA = cvLoadImage(imgAFilename, CV_LOAD_IMAGE_UNCHANGED);
	IplImage* imgB = cvLoadImage(imgBFilename, CV_LOAD_IMAGE_UNCHANGED);
	imgWidth = imgA->width;
	imgHeight = imgA->height;
	
	// Initialise program
	InitLine();
	InitGlut(argc, argv);
	InitGlew();
	InitTexture(imgA, imgB);
	
	// Register GLUT callback functions
	glutDisplayFunc(onRender);
	glutKeyboardFunc(onKeyPress);
	glutReshapeFunc(onResize);

	// Display user instructions in console window.
	printf( "Press and hold 'A/D' to control morphing\n" );
	printf( "Press 'T' to morph between faces\n" );
	printf( "Press 'X' to toggle between blending\n(Cross-dissolve, source only, destination only)\n" );
	printf( "Press 'L' to show lines\n" );
	printf( "Press 'Q' to quit.\n\n" );

	glutMainLoop();
	return 0;
}

static void MakeMorphImage(float t)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

	// Set up projection, modelview matrices and viewport.
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluOrtho2D( 0, imgWidth, 0, imgHeight );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glViewport( 0, 0, imgWidth, imgHeight );

	// Enable morphing shader
	glUseProgram( morphProg );

	// Bind texture to texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texA);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texB);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texLineA);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, texLineB);

	// Set shader uniform vars
	GLint uniStep = glGetUniformLocation( morphProg, "Step" );
	glUniform1f( uniStep, t );
	GLint uniBlendType = glGetUniformLocation( morphProg, "BlendType" );
	glUniform1f( uniBlendType, (float)blendType );

	// Render quads
	glBegin( GL_QUADS );
		glVertex2f( 0, 0 );
		glVertex2f( 0, imgHeight );
		glVertex2f( imgWidth, imgHeight );
		glVertex2f( imgWidth, 0 );
	glEnd();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

static void drawMorphImage()
{
	int leftBorder = (winWidth - imgScale * imgWidth) / 2.0f;
	int bottomBorder = (winHeight - imgScale * imgHeight) / 2.0f;

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, morphedTexObj);
	glBegin( GL_QUADS );
		glTexCoord2f(0, 0);
		glVertex2f(leftBorder, bottomBorder);
		glTexCoord2f(0, 1);
		glVertex2f(leftBorder, bottomBorder + imgHeight * imgScale);
		glTexCoord2f(1, 1);
		glVertex2f(leftBorder + imgWidth * imgScale, bottomBorder + imgHeight * imgScale);
		glTexCoord2f(1, 0);
		glVertex2f(leftBorder + imgWidth * imgScale, bottomBorder);
	glEnd();
}

static void drawLines(float t)
{
	interpolateLines(lineA, lineB, lineInterp, numLines, t);
	int leftBorder = (winWidth - imgScale * imgWidth) / 2.0f;
	int bottomBorder = (winHeight - imgScale * imgHeight) / 2.0f;

	glBegin(GL_LINES);
	for(int i=0; i<numLines; i++)
	{
		glVertex2f(lineInterp[i*4]*imgScale+leftBorder, lineInterp[i*4+1]*imgScale+bottomBorder);
		glVertex2f(lineInterp[i*4+2]*imgScale+leftBorder, lineInterp[i*4+3]*imgScale+bottomBorder);
	}
	glEnd();
}

static void interpolateLines(float* src, float* dest, float* out, int size, float t)
{
	for (int i=0; i<size*4; i++)
		out[i] = src[i] * (1-t) + dest[i] * t;
}

//---------------------------------------------------------------------------
// Check framebuffer status.
// Modified from the sample code provided in the 
// GL_EXT_framebuffer_object extension sepcifications.
//---------------------------------------------------------------------------
static bool CheckFramebufferStatus() 
{
	GLenum status = (GLenum) glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
	switch( status ) 
	{
	case GL_FRAMEBUFFER_COMPLETE_EXT:
		return true;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
		fprintf( stderr, "Framebuffer incomplete, incomplete attachment\n" );
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
		fprintf( stderr, "Framebuffer incomplete, missing attachment\n" );
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
		fprintf( stderr, "Framebuffer incomplete, attached images must have same dimensions\n" );
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
		fprintf( stderr, "Framebuffer incomplete, attached images must have same format\n" );
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
		fprintf( stderr, "Framebuffer incomplete, missing draw buffer\n" );
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
		fprintf( stderr, "Framebuffer incomplete, missing read buffer\n" );
		return false;
	case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
		fprintf( stderr, "Unsupported framebuffer format\n" );
		return false;
	}
	return false;
}