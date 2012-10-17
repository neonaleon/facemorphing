# A program for marking corresponding points in two images.
# Author: Leow Wee Kheng
# Department of Computer Science, National University of Singapore
# Last update: 20 Sep 2012
#
# Display window with 2 images for manual selection of corresponing points
# Click left button to select corner point, alternate between left framebuffer and right framebuffer
# Click right mouse button to remove selection. 
# Select a point in left (right) framebuffer to remove. Then, corresponding point in right 
# (left) framebuffer is also removed.

# Search for lines adjacent to a point
def search_lines(lineList, ptIndex):
	adjacentLines = []
	for i in range(len(lineList)):
		if lineList[i][0] == ptIndex or lineList[i][1] == ptIndex:
			adjacentLines.append(i)
		if len(adjacentLines) >= 2:
			return adjacentLines
	return adjacentLines

# Search for a point in the list and return the index
def search_point(ptList, x, y, radius):
	minx = x - radius
	maxx = x + radius
	miny = y - radius
	maxy = y + radius
	
	for i in range(len(ptList)):
		px = ptList[i][0]
		py = ptList[i][1]
		if px >= minx and px <= maxx and py >= miny and py <= maxy:
			return i
	return -1

# Redraw points in framebuffer
def redraw_lines(window):
	gui = window.gui
	cv.Copy(window.inimg, window.framebuffer)
	for i in range(len(gui.indexBuffer)):
		a = gui.indexBuffer[i][0]
		b = gui.indexBuffer[i][1]
		ptA = (int(gui.vertexBuffer[a][0] + 0.5), int(gui.vertexBuffer[a][1] + 0.5)) 
		ptB = (int(gui.vertexBuffer[b][0] + 0.5), int(gui.vertexBuffer[b][1] + 0.5)) 
		cv.Circle(window.framebuffer, ptA, 2, markcolor, marktype)
		cv.Circle(window.framebuffer, ptB, 2, markcolor, marktype)
		cv.Line(window.framebuffer, ptA, ptB, markcolor, 2)
		cv.PutText(window.framebuffer, str(i + 1), (int(ptA[0] + 5), int(ptA[1] + 5)), font, markcolor)
	cv.ShowImage(window.winname, window.framebuffer)

# Callback function for window
def window_mouse_callback(event, x, y, flags, param):
	window = param
	gui = window.gui
		
	# If this window is the focus, then accept left mouse click
	if event == cv.CV_EVENT_LBUTTONUP:
		# User has stopped dragging
		if gui.dragPt != -1:
			gui.dragPt = -1
			return
		
		# Cache previous point
		if gui.prevPt != -1: prevPt = gui.vertexBuffer[gui.prevPt]
		
		# Left mouse button up from second press of double click
		if gui.prevPt != -1 and (x - prevPt[0]) * (x - prevPt[0]) + (y - prevPt[1]) * (y - prevPt[1]) < 25:
			gui.prevPt = -1
			return
		
		# Add new point into vertex buffer if it is sufficiently far
		currPtIndex = search_point(gui.vertexBuffer, x, y, 15)
		if currPtIndex == -1:
			criteria = (cv.CV_TERMCRIT_ITER | cv.CV_TERMCRIT_EPS, 30, 0.01)
			[(fx, fy)] = cv.FindCornerSubPix(window.gray, [(x, y)], (4, 4), (0, 0), criteria)
			gui.vertexBuffer.append((fx, fy))
			cv.Circle(window.framebuffer, (int(fx + 0.5), int(fy + 0.5)), 2, markcolor, marktype)
			cv.ShowImage(window.winname, window.framebuffer)
			currPtIndex = len(gui.vertexBuffer) - 1
		elif currPtIndex == gui.prevPt:
			return
		
		# Add line segment
		if gui.prevPt != -1:
			currPt = gui.vertexBuffer[currPtIndex]
			print "Add line:", prevPt, currPt
			gui.indexBuffer.append((gui.prevPt, currPtIndex))
			redraw_lines(window)
		gui.prevPt = currPtIndex
		
	# If right button down, then delete selected point
	elif event == cv.CV_EVENT_RBUTTONUP:
		ptIndex = search_point(gui.vertexBuffer, x, y, 4)
		if ptIndex == -1: return
		gui.prevPt = -1
		adjLines = search_lines(gui.indexBuffer, ptIndex)
		for i in reversed(adjLines):
			print "delete:", gui.vertexBuffer[gui.indexBuffer[i][0]], gui.vertexBuffer[gui.indexBuffer[i][1]]
			del gui.indexBuffer[i]
		redraw_lines(window)
		
	# Move points
	elif event == cv.CV_EVENT_MOUSEMOVE and (flags & cv.CV_EVENT_FLAG_LBUTTON):
		if gui.dragPt == -1:
			ptIndex = search_point(gui.vertexBuffer, x, y, 4)
			if ptIndex == -1: return
			gui.dragPt = ptIndex
		criteria = (cv.CV_TERMCRIT_ITER | cv.CV_TERMCRIT_EPS, 30, 0.01)
		[(fx, fy)] = cv.FindCornerSubPix(window.gray, [(x, y)], (4, 4), (0, 0), criteria)
		gui.vertexBuffer[gui.dragPt] = (fx, fy)
		redraw_lines(window)

# Window class and GUI class for organizing the windows
class Window:
	def __init__(self, gui, imgFilename):
		self.gui = gui
		self.winname = imgFilename
		self.window = cv.NamedWindow(self.winname, cv.CV_WINDOW_AUTOSIZE)
		cv.SetMouseCallback(self.winname, window_mouse_callback, self)
		self.inimg = cv.LoadImage(imgFilename, cv.CV_LOAD_IMAGE_COLOR)
		self.framebuffer = cv.LoadImage(imgFilename, cv.CV_LOAD_IMAGE_COLOR)
		self.gray = cv.CreateMat(self.framebuffer.height, self.framebuffer.width, cv.CV_8UC1)
		cv.CvtColor(self.framebuffer, self.gray, cv.CV_RGB2GRAY)
		cv.ShowImage(self.winname, self.framebuffer)
			
class GUI:
	def __init__(self, imgFilename, lineFilename):
		self.vertexBuffer = []
		self.indexBuffer = []
		self.prevPt = -1
		self.dragPt = -1
		self.load_lines(lineFilename)
		self.win = Window(self, imgFilename)
		redraw_lines(self.win)
		
	def load_lines(self, lineFilename):
		try:
			lineFile = open(lineFilename)
		except IOError:
			return
		data = np.genfromtxt(lineFilename)
		lineFile.close()
		for i in range(len(data)):
			self.vertexBuffer.append((data[i, 0], data[i, 1]))
			self.vertexBuffer.append((data[i, 2], data[i, 3]))
			ptA = search_point(self.vertexBuffer, data[i, 0], data[i, 1], 1)
			ptB = search_point(self.vertexBuffer, data[i, 2], data[i, 3], 1)
			self.indexBuffer.append((ptA, ptB))
		
	def save_points(self, lineFilename):
		lineFile = open(lineFilename, "w")
		for (a, b) in self.indexBuffer:
			lineFile.write(str(self.vertexBuffer[a][0]) + " " + str(self.vertexBuffer[a][1]) + " " + str(self.vertexBuffer[b][0]) + " " + str(self.vertexBuffer[b][1]) + "\n")
		lineFile.close()
		
		
#-----------------------------------------
# Main 

import os
import cv2.cv as cv
import numpy as np

rootDir = "./"
imgFile = rootDir + "input.jpg"
lineFile = os.path.splitext(imgFile)[0] + ".mld"
markcolor = (0, 0, 255)
marktype = 2
font = cv.InitFont(cv.CV_FONT_HERSHEY_PLAIN, 1, 1)

# Create and run GUI
gui = GUI(imgFile, lineFile)

while True:	
	if cv.WaitKey(0) == 27:   # Wait for 0 ms and check key press. ASCII 27 is the ESC key.
		break

gui.save_points(lineFile)

# Clean up
cv.DestroyWindow(gui.win.winname)
