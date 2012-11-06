#include "MarkUI.h"
#include "ImageMorph.h"
#include "constants.h"

extern const CvScalar MARKCOLOR;
extern const int CIRCLE_SIZE;

CMarkUI::CMarkUI(CImageMorph *app, const char* filename)
{
	m_app = app;
	m_isModified = true;
	m_prevVertex = -1;
	m_dragPoint = -1;
	//cvInitFont(&FONT, CV_FONT_HERSHEY_SIMPLEX, 1.0, 1.0, 0, 1, CV_AA);

	strcpy(m_imgFilename, filename);
	m_inImage = cvLoadImage(m_imgFilename, CV_LOAD_IMAGE_UNCHANGED);
	m_glImage = cvLoadImage(m_imgFilename, CV_LOAD_IMAGE_UNCHANGED);
	cvCopy(m_inImage, m_glImage);
	cvFlip(m_glImage, 0);
	m_frameBuffer = cvLoadImage(m_imgFilename, CV_LOAD_IMAGE_UNCHANGED);
	string fn = filename;
	string lineFilename = fn.substr(0, fn.find_last_of(".")).append(".mld");
	strcpy(m_lineFilename, lineFilename.c_str());

	loadLines();
	onRedraw();
}

CMarkUI::~CMarkUI(void)
{
	saveLines();
	cvReleaseImage(&m_inImage);
}

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
	return m_glImage->imageData;
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

void CMarkUI::onMousePress( int event, int x, int y, int flags, void* param )
{
	int currPtIndex;
	vector<int> adjacentLines;
	y = m_inImage->height - y;

	switch( event ){
	case CV_EVENT_LBUTTONUP:
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
		onRedraw();
		break;

	case CV_EVENT_LBUTTONDOWN:
		m_dragPoint = searchPoint(x, y, 5);
		break;

	case CV_EVENT_MOUSEMOVE:
		if(!(flags & CV_EVENT_FLAG_LBUTTON))
			return;
		if(m_dragPoint == -1)
			return;

		// End line segment
		m_prevVertex = -1;

		m_vertexBuffer[m_dragPoint].x = x;
		m_vertexBuffer[m_dragPoint].y = y;

		// Inform app of line change
		m_isModified = true;
		//m_app->onLineUpdate();
		onRedraw();
		break;

	case CV_EVENT_RBUTTONUP:
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
		onRedraw();
		break;
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
	return m_frameBuffer;
}

void CMarkUI::onRedraw()
{
	// Clear framebuffer
	cvCopy(m_inImage, m_frameBuffer);

	// Draw first point
	if(m_prevVertex != -1)
	{
		CvPoint first = cvPointFrom32f(m_vertexBuffer[m_prevVertex]);
		first.y = m_inImage->height - first.y;
		cvCircle(m_frameBuffer, first, CIRCLE_SIZE, MARKCOLOR);
	}

	// Draw all other lines and points
	for(int i=0; i<m_indexBuffer.size(); i++)
	{
		CvPoint start = cvPointFrom32f(m_vertexBuffer[m_indexBuffer[i].start]);
		start.y = m_inImage->height - start.y;
		CvPoint end = cvPointFrom32f(m_vertexBuffer[m_indexBuffer[i].end]);
		end.y = m_inImage->height - end.y;
		cvCircle(m_frameBuffer, start, CIRCLE_SIZE, MARKCOLOR);
		cvCircle(m_frameBuffer, end, CIRCLE_SIZE, MARKCOLOR);
		cvLine(m_frameBuffer, start, end, MARKCOLOR);
		char number[31];
		itoa(i+1, number, 10);
		CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 0.8, 0.8);
		cvPutText(m_frameBuffer, number, start, &font, MARKCOLOR);
	}
}
