#pragma once

#include <vector>
#include <cv.h>
#include <highgui.h>

using namespace std;

class CImageMorph;

class CMarkUI
{
private:
	struct Line2D
	{
		int start, end;
	};

private:
	vector<CvPoint2D32f> m_vertexBuffer;
	vector<Line2D> m_indexBuffer;
	vector<float> m_packedLine;

	char m_imgFilename[31];
	char m_lineFilename[31];

	IplImage* m_inImage;
	IplImage* m_glImage;
	IplImage* m_frameBuffer;

	bool m_isModified;
	int m_prevVertex;
	int m_dragPoint;

	CImageMorph *m_app;

	// static CvFont FONT;

public:
	float* getPackedLine();
	char* getImageData();
	IplImage* getImage();
	int getNumLines();

	void loadLines();
	void saveLines();

	void onMousePress(int event, int x, int y, int flags, void* param);

	CMarkUI(CImageMorph *app, const char* filename);
	~CMarkUI(void);

private:
	int searchPoint(float x, float y, float radius);
	void searchLines(int pointIndex, vector<int>* adjacentLines);
	void onRedraw();
};

