/////////////////////////////////////////////////////////////////////////////
// Image morphing program
//
// Author: Daniel Seah
/////////////////////////////////////////////////////////////////////////////

// Program paramters
const char* imgAFilename = "inputa.jpg";
const char* imgBFilename = "inputb.jpg";
const char* lineAFilename = "inputa.mld";
const char* lineBFilename = "inputb.mld";
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
static GLuint shaderProg;

// GLUT window handle.
static GLuint glutWindowHandle;
int imgWidth, imgHeight;

// Texture image handle
GLuint texA, texB, texLineA, texLineB;

// Deug stuff
//GLuint outputTex;
//float* outputArray;

// Line lists
int numLines;
float* lineA;
float* lineB;

int frameNumber, frameTotal;
int blendType;
bool isPlaying;
int playDirection;
int lastTime;

// Forward declarations
static bool CheckFramebufferStatus();
static void onRender();
static void onUpdate(int value);


//---------------------------------------------------------------------------
// Keyboard callback function
//---------------------------------------------------------------------------
void onKeyPress( unsigned char key, int x, int y )
{
	switch ( key )
	{
	case 't':
	case 'T':
		isPlaying = !isPlaying;
		lastTime =  glutGet(GLUT_ELAPSED_TIME);
		if(frameNumber <= 0)
			playDirection = 1;
		else if(frameNumber >= frameTotal)
			playDirection = -1;
		glutTimerFunc(500.0f / frameRate, onUpdate, 0);	// Nyquist magic
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

		// Quit program.
	case 'q':
	case 'Q': 
	case 27:
		glutDestroyWindow(glutWindowHandle);
		exit(0);
		break;
	}
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
	// Bind texture to texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texA);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texB);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texLineA);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, texLineB);

	// Calculate frame position and thus t
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	int elapsed = currentTime - lastTime;
	float t = frameNumber;
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
	t = float(frameNumber) / frameTotal;

	// Set shader uniform vars
	GLint uniStep = glGetUniformLocation( shaderProg, "Step" );
	glUniform1f( uniStep, t );
	GLint uniBlendType = glGetUniformLocation( shaderProg, "BlendType" );
	glUniform1i( uniBlendType, blendType );

	// Render quads
	glBegin( GL_QUADS );
	glVertex2f( 0, 0 );
	glVertex2f( 0, imgHeight );
	glVertex2f( imgWidth, imgHeight );
	glVertex2f( imgWidth, 0 );
	glEnd();

	glutSwapBuffers();
	lastTime = currentTime;
	/*glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glReadBuffer( GL_COLOR_ATTACHMENT0_EXT );
	glReadPixels( 0, 0, imgWidth, imgHeight, GL_RGBA, GL_FLOAT, outputArray );*/
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

	// Set image parameters
	GLint uniTexA = glGetUniformLocation( shaderProg, "TexA" );
	glUniform1i( uniTexA, 0 );
	GLint uniTexB = glGetUniformLocation( shaderProg, "TexB" );
	glUniform1i( uniTexB, 1 );
	GLint uniTexWidthLoc = glGetUniformLocation( shaderProg, "TexWidth" );
	glUniform1i( uniTexWidthLoc, imgWidth );
	GLint uniTexHeightLoc = glGetUniformLocation( shaderProg, "TexHeight" );
	glUniform1i( uniTexHeightLoc, imgHeight );

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

	// Set line parameters
	GLint uniLineA = glGetUniformLocation( shaderProg, "ALines" );
	glUniform1i( uniLineA, 2 );
	GLint uniLineB = glGetUniformLocation( shaderProg, "BLines" );
	glUniform1i( uniLineB, 3 );
	GLint uniLineCount = glGetUniformLocation( shaderProg, "LineCount" );
	glUniform1i( uniLineCount, numLines );

	//-----------------------------------------------------------------------------
	// Attach the two textures to a FBO.
	//-----------------------------------------------------------------------------
	/*glActiveTexture( GL_TEXTURE4 );
	glGenTextures( 1, &outputTex );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, outputTex );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB,
		imgWidth, imgHeight, 0, GL_RGBA, GL_FLOAT, NULL );
	printOpenGLError();
	GLuint fbo;
	glGenFramebuffersEXT( 1, &fbo ); 
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, fbo );

	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
		GL_TEXTURE_RECTANGLE_ARB, outputTex, 0 );
	CheckFramebufferStatus();
	printOpenGLError();
	glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );*/
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
	glutInitWindowSize (imgWidth, imgHeight);

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
	glPolygonMode( GL_FRONT, GL_FILL );

	// Set up projection, modelview matrices and viewport.
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluOrtho2D( 0, imgWidth, 0, imgHeight );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glViewport( 0, 0, imgWidth, imgHeight );

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
	if ( !GLEW_ARB_texture_float || 
		!GLEW_ARB_texture_rectangle)
	{
		fprintf( stderr, "Error: Some necessary OpenGL extensions are not supported.\n" );
		char ch; scanf( "%c", &ch ); // Prevents the console window from closing.
		exit( 1 );
	}

	// Create shader program object.
	shaderProg = makeShaderProgramFromFiles( vertShaderFilename, fragShaderFilename, NULL );
	if ( shaderProg == 0 )
	{
		fprintf( stderr, "Error: Cannot create shader program object.\n" );
		char ch; scanf( "%c", &ch ); // Prevents the console window from closing.
		exit( 1 );
	}

	// Deploy user-defined shaders.
	glUseProgram( shaderProg );
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
}

int main(int argc, char* argv[])
{
	// Init global variables
	blendType = 0;
	frameNumber = 0;
	frameTotal = frameRate * duration;
	isPlaying = false;
	playDirection = 1;

	// Load images
	IplImage* imgA = cvLoadImage(imgAFilename, CV_LOAD_IMAGE_UNCHANGED);
	IplImage* imgB = cvLoadImage(imgBFilename, CV_LOAD_IMAGE_UNCHANGED);
	imgWidth = imgA->width;
	imgHeight = imgA->height;
	
	//outputArray = new float[imgWidth * imgHeight * 4];
	InitLine();
	InitGlut(argc, argv);
	InitGlew();
	InitTexture(imgA, imgB);
	
	// Register GLUT callback functions
	glutDisplayFunc(onRender);
	glutKeyboardFunc(onKeyPress);

	// Display user instructions in console window.
	printf( "Press and hold 'A/D' to control morphing\n" );
	printf( "Press 'T' to morph between faces\n" );
	printf( "Press 'X' to toggle between blending\n(Cross-dissolve, source only, destination only)\n" );
	printf( "Press 'Q' to quit.\n\n" );

	glutMainLoop();
	return 0;
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