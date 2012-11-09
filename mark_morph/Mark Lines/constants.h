/////////////////////////////////////////////////////////////////////////////
// File: constants.cpp
// 
// Program settings
// Feel free to edit this file
//
// Author: Daniel Seah
/////////////////////////////////////////////////////////////////////////////

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
const float MARKCOLOR[] = {0.5, 0.8, 0.15686};
const float CIRCLE_SIZE = 4;