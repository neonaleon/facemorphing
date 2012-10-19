######################
#	VECTOR FUNCTIONS	#
######################

################################################################################
# Function name: perpendicular
# Description:	Calculates the vector perpendicular to the input vector
################################################################################
def perpendicular(vector):
	return np.matrix([[-vector[1,0]], [vector[0,0]]])

#####################
#	MORPH FUNCTIONS	#
#####################

################################################################################
# Function name: morphMain
# Parameters:
#		 -sourceImage: the original image before morphing
#		 -destinationImage: the final image we are morphing to
#		 -sourceLines: the feature lines in the source image
#		 -destinationLines: the feature lines in the destination image
# Description:
#		A main function that sets up everything needed to perform the morph
#		including number of frames, output folder, calculation of line 
#		interpolation, the call to the morph function, and writing the new
#		images out to files.	Also prints status information for each frame 
#		including frame number and feature lines.
################################################################################
def morphMain():
	#loop through number of frames and call morphing function
	for i in range(int(framerate * duration) + 1):
		t = i / framerate * duration
		#calculate interpolated lines
		interpLines = interpolateLines(aLines, bLines, t)

		#call the morph function and save result into new image
		morphImage = imageMorph(imgA, imgB, aLines, interpLines, bLines, t)
		#write the current frame to file
		cv.writeFrame(outvid, morphImage)

################################################################################
# Function name: imageMorph
# Parameters:
#		 -sourceImage: the original image before morphing
#		 -destinationImage: the final image we are morphing to
#		 -sourceLines: the feature lines in the source image
#		 -interpLines: the intermediate feature lines
#		 -destinationLines: the feature lines in the destination image
#		 -numLines: the total number of feature lines
#		 -currFrameNum: the current frame being processed
#		 -totalFrameNum: the total number of frames in the morph
# Return:
#		A new image that is warped and cross dissolved according to the 
#		attributes for the current frame
# Description:
#		A morphing function based on the algorithm created by Beier and Neely 
#		as described in the 1992 SIGGRAPH paper "Feature-Based Image 
#		Metamorphosis" (http://www.hammerhead.com/thad/morph.html).
#		The technique uses feature lines in the source image and destination 
#		image to warp the two images towards the lines while cross-dissolving 
#		the images to blend them over time.
################################################################################
def imageMorph(sourceImage, destinationImage, sourceLines, interpLines, destinationLines, t):
	#CONSTANTS
	a = 0.5 #smoothness of warping
	b = 1.25 #relative line strength
	p = 0.25 #line size strength

	#get width and height of each image
	width = sourceImage.width
	height = sourceImage.height

	#create new blank image
	output = cv.CreateMat(height, width, cv.CV_8UC3)

	#loop through the pixels of both images
	for x in range(width):
		for y in range(height):

			#get pixel values
			X = np.matrix([[x], [y]]) #get x and y for the current pixel

			#initialize weight and displacement values
			DSUMA = np.matrix([[0], [0]]) #intialize source displacement sum to 0
			DSUMB = np.matrix([[0], [0]]) #intialize destination displacement sum to 0
			weightsum = 0 #intialize source weight sum to 0
			weightsumB = 0 #intialize destination weight sum to 0

			#loop through list of lines and process each feature line in the images
			for i in range(len(sourceLines)):

				#set the vector for the current line we are warping towards
				P = interpLines[i][0] #destination line start point
				Q = interpLines[i][1] #destination line end point
				PQ = Q - P #define destination line segment

				#calculate the u and v to warp the destination image
				u = calcU(X, P, Q) #u is the value along the feature line
				v = calcV(X, P, Q) #v is the perpendicular distance of the current pixel to the line

				#set the vector for the current line in the source image we are warping from
				Pprime = sourceLines[i][0] #source line start point
				Qprime = sourceLines[i][1] #source line start point

				#calculate the new pixel x and y values for the current source image pixel due to warping
				Xprime = calcXPrime(Pprime, Qprime, u, v)

				#set the vector for the current line in destinationImage
				Pprime2 = destinationLines[i][0] #destination line start point
				Qprime2 = destinationLines[i][1] #destination line end point

				#calculate the new pixel x and y values for the current destination image pixel due to warping
				Xprime2 = calcXPrime(Pprime2, Qprime2, u, v)

				#find how far new pixels are from current pixels in the source and destination images
				displacement = Xprime - X #source image displacement
				displacement2 = Xprime2 - X #destination image displacement

				#check to see if the effect of u
				#if u is greater than 1, distance is the length of the line from the pixel to the end point
				if(u >= 1):
					dist = la.norm(Q - X)
				#if u is less than 0, distance is the length of the line from the pixel to the start point
				elif(u <= 0):
					dist = la.norm(P - X)
				#otherwise distance is absolute value of v
				else:
					dist = abs(v)

				#find out contribution of current line to pixel warping
				#determine new xy's contribution based on its distance from the line
				length = la.norm(PQ) #length of current line
				weight = math.pow(length, p)/(a + dist)
				weight = math.pow(weight, b) #weight of current line

				#add the displacement sum for source and destination
				DSUMA += displacement * weight
				DSUMB = displacement2 * weight

				#add weight to weightsume
				weightsum += weight

				#adjust displacement sum by weightsum
				DSUMweightsum = DSUMA / weightsum
				DSUMweightsum2 = DSUMB / weightsum

				#adjust new source and destination pixels by displacement sums
				Xprime = X + DSUMweightsum
				Xprime2 = X + DSUMweightsum2

				#get x and y values for new source pixel
				newSourceX = int(Xprime[0])
				newSourceY = int(Xprime[1])

				#get x and y values for new destination pixel
				newDestinationX = int(Xprime2[0])
				newDestinationY = int(Xprime2[1])

				#if new source xy in range of source image, get that pixel
				if(newSourceX in range(width) and newSourceY in range(height)):
					sourcePixel = sourceImage[newSourceX, newSourceY]
				#otherwise get current source image pixel
				else:
					sourcePixel = sourceImage[x, y]

				#if new destination xy in range of destination image, get that pixel
				if(newDestinationX in range(width) and newDestinationY in range(height)):
					destPixel = destinationImage[newDestinationX, newDestinationY]
				#otherwise get current destination image pixel
				else:
					destPixel = destinationImage[x, y]

				#cross-dissolve source and destination images by the current weights due to current frame number
				r = sourcePixel[0]*t + destPixel[0]*(1-t)
				g = sourcePixel[1]*t + destPixel[1]*(1-t)
				b = sourcePixel[2]*t + destPixel[2]*(1-t)
	
				#set the new image's pixel to the cross-dissolved, warped pixel color
				output[x, y] = [r, g, b]


	#return the result
	return output


################################################################################
# Function name: interpolateLines
# Parameters:
#		-sourceLines: the list of source lines
#		-destinationLines: the list of destination lines
#		-numLines: the total number of feature lines
#		-currFrame: the current frame being morphed
#		-totalFrames: the total number of frames created
# Return:
#		An intermediate set of lines
# Description:
#		Uses the start and end points of the source and destination lines,
#		as well as the frame information to create an intermediate set of 
#		lines that is used to move the source lines to the destination lines
#		over a series of frames
################################################################################
def interpolateLines(sourceLines, destinationLines, t):
	#initialize newLines to number of line segments needed
	newLines = []

	for i in range(len(sourceLines)):
		ptA = sourceLines[i][0] * t + destinationLines[i][0] * (1 - t)
		ptB = sourceLines[i][1] * t + destinationLines[i][1] * (1 - t)
		newLines.append((ptA, ptB))
	return newLines


################################################################################
# Function name: calcXPrime
# Parameters:
#		-Pprime: the source line start point
#		-Qprime: the source line end point
#		-u: the distance along the feature line
#		-v: the perpendicular distance from the source pixel to the feature line
# Return:
#		The distance along the feature line
# Description:
#		Uses the Beier-Neely equation to calculate the new pixel value due 
#		to warping using various vector operations.	If the new value is 
#		out of the range of the image dimensions, the current value is used
################################################################################
def calcXPrime(Pprime, Qprime, u, v):
	QprimePprime = Qprime - Pprime
	QprimePprimePerp = perpendicular(QprimePprime)
	top = QprimePprimePerp * v
	bottom = la.norm(QprimePprime)
	last = top / bottom
	mid = QprimePprime * u
	value = np.matrix([[Pprime[0,0] + mid[0,0] + last[0,0]], [Pprime[1,0] + mid[1,0] + last[1,0]]])
	return value

################################################################################
# Function name: calcU
# Parameters:
#		-X: the current destination image pixel
#		-P: the destination line start point
#		-Q: the destination line end point
# Return:
#		The distance along the feature line
# Description:
#		Uses the Beier-Neely equation to calculate the distance along the 
#		feature line using various vector operations.	The value is between
#		0 and 1 if it lies along the line, otherwise u is outside the range
################################################################################
def calcU(X, P, Q):
	XP = P - X
	PQ = P - Q
	top = np.dot(XP.T, PQ)
	bottom = np.dot(PQ.T, PQ)
	value = top/bottom
	return value	

################################################################################
# Function name: calcV
# Parameters:
#		-X: the current destination image pixel
#		-P: the destination line start point
#		-Q: the destination line end point
# Return:
#		The perpendicular distance from the current pixel to the line
# Description:
#		Uses the Beier-Neely equation to calculate the perpendicular distance
#		of the current destination pixel to the current feature line using 
#		various vector operations
################################################################################
def calcV(X, P, Q):
	XP = X - P # not p - x?
	PQ = Q - P
	PQperp = perpendicular(PQ)
	top = np.dot(XP.T, PQperp)
	bottom = la.norm(PQ)
	value = top/bottom
	return value


def readLineData(filename):
	lineFile = open(filename)
	data = np.genfromtxt(lineFile)
	lineFile.close()
	lineList = []
	for (x, y, u ,v) in data:
		ptA = np.matrix([[x], [y]])
		ptB = np.matrix([[u], [v]])
		lineList.append((ptA, ptB))
	return lineList

import os
import math
import numpy as np
import numpy.linalg as la
import cv2.cv as cv

rootDir = "./"
imageA = "inputa.jpg"
imageB = "inputb.jpg"
outVideo = "out.avi"
framerate = 24
duration = 2.0

imgAFilename = rootDir + imageA
lineAFilename = os.path.splitext(imgAFilename)[0] + ".mld"
rootDir = "./"
imgBFilename = rootDir + imageB
lineBFilename = os.path.splitext(imgBFilename)[0] + ".mld"
outFilename = rootDir + outVideo

imgA = cv.LoadImage(imgAFilename)
imgB = cv.LoadImage(imgBFilename)
aLines = readLineData(lineAFilename)
bLines = readLineData(lineBFilename)
outvid = cv.CreateVideoWriter(outFilename, 0, framerate, (imgA.width, imgA.height), 1)

#check if images are the same size
if(imgA.width != imgB.width or imgA.height != imgB.height):
	print "Cannot morph image of different sizes"

morphMain()

del imgA
del imgB
del outvid



