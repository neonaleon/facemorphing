#include "Renderer.h"
#include "constants.h"
#include "MarkUI.h"
#include "shader_util.h"


// Renderer defines
//extern const char* OUTVIDEO;
extern const int FRAMERATE;
extern const int DURATION;
//extern const char VERTSHADER;
//extern const char FRAGSHADER;
extern const int CODEC;

CRenderer::CRenderer(CMarkUI* imgA, CMarkUI* imgB)
{
	m_pImageA = imgA;
	m_pImageB = imgB;
	m_width = m_pImageA->getImage()->width;
	m_height = m_pImageA->getImage()->height;

	// Initialise renderer
	initGlut();
	initGlew();
	initTexture();
}

CRenderer::~CRenderer(void)
{
}

void CRenderer::initGlut()
{
	// Initialize GLUT.
	int argc = 1;
	char *argv[] = {"Renderer", NULL};
	glutInit( &argc, argv );
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize (128, 128);

	// This creates a OpenGL rendering context so that
	// we can start to issue OpenGL commands after this.
	glutCreateWindow( "Renderer" );  

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
// Create a OpenGL rendering context. 
// Check for OpenGL 2.0 and the necessary OpenGL extensions. 
// Read in the shaders from files to create a shader program object.
//---------------------------------------------------------------------------
void CRenderer::initGlew()
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
		m_width, m_height, 0, GL_BGR, GL_UNSIGNED_BYTE, m_pImageA->getImageData());
	printOpenGLError();

	// Create texture B.
	glActiveTexture( GL_TEXTURE1 );
	glGenTextures( 1, &m_texA );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, m_texA );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB,
		m_width, m_height, 0, GL_BGR, GL_UNSIGNED_BYTE, m_pImageB->getImageData());
	printOpenGLError();

	// Set image parameters in shader
	GLint uniTexA = glGetUniformLocation( m_morphProg, "TexA" );
	glUniform1i( uniTexA, 0 );
	GLint uniTexB = glGetUniformLocation( m_morphProg, "TexB" );
	glUniform1i( uniTexB, 1 );
	GLint uniTexWidthLoc = glGetUniformLocation( m_morphProg, "TexWidth" );
	glUniform1f( uniTexWidthLoc, (float)m_width );
	GLint uniTexHeightLoc = glGetUniformLocation( m_morphProg, "TexHeight" );
	glUniform1f( uniTexHeightLoc, (float)m_height );

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
		m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL );
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
	gluOrtho2D( 0, m_width, 0, m_height );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glViewport( 0, 0, m_width, m_height );

	// Enable morphing shader
	glUseProgram( m_morphProg );

	// Bind texture to texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_texA);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_texB);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_texLineA);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_texLineB);
	
	// Set shader uniform vars
	GLint uniStep = glGetUniformLocation( m_morphProg, "Step" );
	glUniform1f( uniStep, t );

	// Render quads
	glBegin( GL_QUADS );
	glVertex2f( 0, 0 );
	glVertex2f( 0, m_height );
	glVertex2f( m_width, m_height );
	glVertex2f( m_width, 0 );
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
	float *a = m_pImageA->getPackedLine();
	float *b = m_pImageB->getPackedLine();
	int numLines = m_pImageA->getNumLines();

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

	// Set line count in shader
	GLint uniLineCount = glGetUniformLocation( m_morphProg, "LineCount" );
	glUniform1f( uniLineCount, (float)numLines );
}

void CRenderer::getRender(char* data)
{
	glFinish();
	glBindTexture(GL_TEXTURE_2D, m_morphedTexObj);
	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
}

void CRenderer::setBlendType(int blendType)
{
	GLint uniBlendType = glGetUniformLocation( m_morphProg, "BlendType" );
	glUniform1f( uniBlendType, (float)blendType );
}
