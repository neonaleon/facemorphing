#include "Renderer.h"
#include "constants.h"
#include "MarkUI.h"
#include "shader_util.h"
#include "GLUTWindow.h"
#include "ImageMorph.h"

// Renderer defines
//extern const char* OUTVIDEO;
extern const int FRAMERATE;
extern const int DURATION;
//extern const char VERTSHADER;
//extern const char FRAGSHADER;
extern const int CODEC;

CRenderer::CRenderer(CImageMorph *app, CMarkUI* imgA, CMarkUI* imgB)
{
	m_app = app;
	m_pImageA = imgA;
	m_pImageB = imgB;
	m_imgWidth = m_pImageA->getImage()->width;
	m_imgHeight = m_pImageA->getImage()->height;

	// Init application states
	m_blendType = 0;
	m_imgScale = 1;
	m_frameNumber = 0;
	m_frameTotal = FRAMERATE * DURATION;
	m_isPlaying = false;
	m_playDirection = 1;
	//m_isConsistent = false;
	m_showDebugLines = false;

	// Initialise renderer
	initGLState();
	initShader();
	initTexture();
}

CRenderer::~CRenderer(void)
{
}

void CRenderer::initGLState()
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
// Read in the shaders from files to create a shader program object.
//---------------------------------------------------------------------------
void CRenderer::initShader()
{
	// Create shader program object.
	m_morphProg = makeShaderProgramFromFiles( VERTSHADER, FRAGSHADER, NULL );
	if ( m_morphProg == 0 )
	{
		fprintf( stderr, "Error: Cannot create shader program object.\n" );
		char ch; scanf( "%c", &ch ); // Prevents the console window from closing.
		exit( 1 );
	}

	// Deploy user-defined shaders.
	glUseProgram( m_morphProg );
}

//---------------------------------------------------------------------------
// Upload both face images and line information to GPU
//---------------------------------------------------------------------------
void CRenderer::initTexture()
{
	// Create texture A.
	glActiveTexture( GL_TEXTURE0 );
	glGenTextures( 1, &m_texA );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, m_texA );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB,
		m_imgWidth, m_imgHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, m_pImageA->getImageData());
	printOpenGLError();

	// Create texture B.
	glActiveTexture( GL_TEXTURE1 );
	glGenTextures( 1, &m_texB );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, m_texB );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB,
		m_imgWidth, m_imgHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, m_pImageB->getImageData());
	printOpenGLError();

	// Set image parameters in shader
	GLint uniTexA = glGetUniformLocation( m_morphProg, "TexA" );
	glUniform1i( uniTexA, 0 );
	GLint uniTexB = glGetUniformLocation( m_morphProg, "TexB" );
	glUniform1i( uniTexB, 1 );
	GLint uniTexWidthLoc = glGetUniformLocation( m_morphProg, "TexWidth" );
	glUniform1f( uniTexWidthLoc, (float)m_imgWidth );
	GLint uniTexHeightLoc = glGetUniformLocation( m_morphProg, "TexHeight" );
	glUniform1f( uniTexHeightLoc, (float)m_imgHeight );

	// Upload line A to GPU
	glActiveTexture( GL_TEXTURE2 );
	glGenTextures( 1, &m_texLineA );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, m_texLineA );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB,
		m_pImageA->getNumLines(), 1, 0, GL_RGBA, GL_FLOAT, m_pImageA->getPackedLine());
	printOpenGLError();

	// Upload line B to GPU
	glActiveTexture( GL_TEXTURE3 );
	glGenTextures( 1, &m_texLineB );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, m_texLineB );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB,
		m_pImageB->getNumLines(), 1, 0, GL_RGBA, GL_FLOAT, m_pImageB->getPackedLine() );
	printOpenGLError();

	// Set line parameters in shader
	GLint uniLineA = glGetUniformLocation( m_morphProg, "ALines" );
	glUniform1i( uniLineA, 2 );
	GLint uniLineB = glGetUniformLocation( m_morphProg, "BLines" );
	glUniform1i( uniLineB, 3 );
	GLint uniLineCount = glGetUniformLocation( m_morphProg, "LineCount" );
	glUniform1f( uniLineCount, (float)m_pImageB->getNumLines() );

	// Create image output texture
	glActiveTexture( GL_TEXTURE4 );
	glGenTextures( 1, &m_morphedTexObj );
	glBindTexture( GL_TEXTURE_2D, m_morphedTexObj );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8,
		m_imgWidth, m_imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL );
	printOpenGLError();

	//-----------------------------------------------------------------------------
	// Attach the output texture to a FBO
	//-----------------------------------------------------------------------------
	glGenFramebuffersEXT( 1, &m_fbo ); 
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_fbo );
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
		GL_TEXTURE_2D, m_morphedTexObj, 0 );
	checkFramebufferStatus();
	printOpenGLError();
};

void CRenderer::makeMorphImage(float t)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);

	// Set up projection, modelview matrices and viewport.
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluOrtho2D( 0, m_imgWidth, 0, m_imgHeight );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glViewport( 0, 0, m_imgWidth, m_imgHeight );

	// Enable morphing shader
	glUseProgram( m_morphProg );

	// Bind texture to texture units
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_texA);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_texB);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_texLineA);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_texLineB);
	
	// Set shader uniform vars
	GLint uniStep = glGetUniformLocation( m_morphProg, "Step" );
	glUniform1f( uniStep, t );
	GLint uniLineCount = glGetUniformLocation( m_morphProg, "LineCount" );
	glUniform1f( uniLineCount, (float)m_pImageA->getNumLines() );

	// Render quads
	glBegin( GL_QUADS );
	glVertex2f( 0, 0 );
	glVertex2f( 0, m_imgHeight );
	glVertex2f( m_imgWidth, m_imgHeight );
	glVertex2f( m_imgWidth, 0 );
	glEnd();

	// Restore output framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

//---------------------------------------------------------------------------
// Check framebuffer status.
// Modified from the sample code provided in the 
// GL_EXT_framebuffer_object extension sepcifications.
//---------------------------------------------------------------------------
bool CRenderer::checkFramebufferStatus() 
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

void CRenderer::setLines()
{
	glutSetWindow(m_window->getWindow());
	float *a = m_pImageA->getPackedLine();
	float *b = m_pImageB->getPackedLine();
	int numLines = m_pImageA->getNumLines();
	glEnable(GL_TEXTURE_2D);

	// Upload line A to GPU
	glActiveTexture( GL_TEXTURE2 );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, m_texLineA );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB,
		numLines, 1, 0, GL_RGBA, GL_FLOAT, a);
	printOpenGLError();

	// Upload line B to GPU
	glActiveTexture( GL_TEXTURE3 );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, m_texLineB );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB,
		numLines, 1, 0, GL_RGBA, GL_FLOAT, b);
	printOpenGLError();

	glutPostRedisplay();
}

void CRenderer::getRender(char* data)
{
	glFinish();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_morphedTexObj);
	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
}

void CRenderer::setBlendType(int blendType)
{
	glUseProgram(m_morphProg);
	GLint uniBlendType = glGetUniformLocation( m_morphProg, "BlendType" );
	glUniform1f( uniBlendType, (float)blendType );
}

void CRenderer::onKeyPress( unsigned char key, int x, int y )
{
	glutSetWindow(m_window->getWindow());

	switch ( key )
	{
	case 'r':
	case 'R':
		m_isPlaying = !m_isPlaying;
		m_showDebugLines = false;
		printf("Rendering to %s...\nDo not close window!\n", OUTVIDEO);
		m_app->writeVideo();
		printf("Render complete\n");
		break;

	case 't':
	case 'T':
		m_isPlaying = !m_isPlaying;
		m_lastTime =  glutGet(GLUT_ELAPSED_TIME);
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
		setBlendType(m_blendType);
		glutPostRedisplay();
		break;

	case 'a':
	case 'A':
		m_frameNumber--;
		if(m_frameNumber < 0)
			m_frameNumber = 0;
		glutPostRedisplay();
		break;

	case 'd':
	case 'D':
		m_frameNumber++;
		if(m_frameNumber > m_frameTotal)
			m_frameNumber = m_frameTotal;
		glutPostRedisplay();
		break;

	case 'l':
	case 'L':
		m_showDebugLines = !m_showDebugLines;
		glutPostRedisplay();
		break;

		// Quit program.
	case 'q':
	case 'Q': 
	case 27:
		exit(0);
		break;
	}
}

void CRenderer::onResize( int width, int height )
{
	int winWidth = m_window->getWidth();
	int winHeight = m_window->getHeight();

	// Determine image scale
	m_imgScale = (float)winWidth / m_imgWidth;
	if(winHeight < m_imgHeight * m_imgScale)
		m_imgScale = (float)winHeight / m_imgHeight;
}

void CRenderer::onRender()
{
	// Calculate t
	float t = (float)m_frameNumber / m_frameTotal;

	makeMorphImage(t);

	// Reset projection, modelview matrices and viewport.
	int winWidth = m_window->getWidth();
	int winHeight = m_window->getHeight();
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

	// Render morphed image
	drawMorphImage();

	// Render debug lines
	if(m_showDebugLines)
		drawLines(t);

	printOpenGLError();
	glutSwapBuffers();
}

void CRenderer::drawLines(float t)
{
	float* lineA = m_pImageA->getPackedLine();
	float* lineB = m_pImageB->getPackedLine();
	int numLines = m_pImageA->getNumLines();
	int leftBorder = (m_window->getWidth() - m_imgScale * m_imgWidth) / 2.0f;
	int bottomBorder = (m_window->getHeight() - m_imgScale * m_imgHeight) / 2.0f;
	float ax, ay, bx, by;

	glDisable(GL_TEXTURE_2D);
	glColor3fv(MARKCOLOR);
	glBegin(GL_LINES);
	for(int i=0; i<numLines; i++)
	{
		ax = lineA[i*4] * (1-t) + lineB[i*4] * t;
		ay = lineA[i*4+1] * (1-t) + lineB[i*4+1] * t;
		bx = lineA[i*4+2] * (1-t) + lineB[i*4+2] * t;
		by = lineA[i*4+3] * (1-t) + lineB[i*4+3] * t;
		glVertex2f(ax*m_imgScale+leftBorder, ay*m_imgScale+bottomBorder);
		glVertex2f(bx*m_imgScale+leftBorder, by*m_imgScale+bottomBorder);
	}
	glEnd();
}

void CRenderer::drawMorphImage()
{
	int leftBorder = (m_window->getWidth() - m_imgScale * m_imgWidth) / 2.0f;
	int bottomBorder = (m_window->getHeight() - m_imgScale * m_imgHeight) / 2.0f;

	glEnable(GL_TEXTURE_2D);
	glActiveTexture( GL_TEXTURE0 );
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBindTexture( GL_TEXTURE_2D, m_morphedTexObj);
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
}

void CRenderer::onUpdate()
{
	if(!m_isPlaying)
		return;

	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	int elapsed = currentTime - m_lastTime;
	if(elapsed < 1000.0f / FRAMERATE)
		return;
	m_lastTime = currentTime;

	m_frameNumber += elapsed / 1000.0f * m_playDirection *FRAMERATE;
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
	glutPostRedisplay();
}
