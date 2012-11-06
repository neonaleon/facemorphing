#pragma once

#include <cv.h>
#include <highgui.h>

// Program parameters
const char IMAGEA[] = "inputa.jpg";
const char IMAGEB[] = "inputb.jpg";
const char OUTVIDEO[] = "outvid.avi";
const int FRAMERATE = 24;
const int DURATION = 3;
const int CODEC = 0;

// Shaders' filenames.
const char VERTSHADER[] = "morph.vert";
const char FRAGSHADER[] = "morph.frag";

// Drawing parameters
const CvScalar MARKCOLOR = cvScalar(0, 0, 255);
const int CIRCLE_SIZE = 1;