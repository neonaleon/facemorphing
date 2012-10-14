# A program for marking corresponding points in two images.
# Author: Leow Wee Kheng
# Department of Computer Science, National University of Singapore
# Last update: 20 Sep 2012
#
# Display window with 2 images for manual selection of corresponing points
# Click left button to select corner point, alternate between left image and right image
# Click right mouse button to remove selection. 
# Select a point in left (right) image to remove. Then, corresponding point in right 
# (left) image is also removed.

# Search for a point in a list and return the index
def search_point(ptlist, x, y, radius):
    minx = x - radius
    maxx = x + radius
    miny = y - radius
    maxy = y + radius

    for i in range(len(ptlist)):
        px = ptlist[i][0]
        py = ptlist[i][1]
        if px >= minx and px <= maxx and py >= miny and py <= maxy:
		   return i, px, py
        else:
		   continue

    return -1, -1, -1

# Redraw points in image
def redraw_points(window):
    cv.Copy(window.inimg, window.image)
    ptlist = window.gui.ptlist[window.side]
    for i in range(len(ptlist)):
        x = int(ptlist[i][0]+0.5)
        y = int(ptlist[i][1]+0.5)
        cv.Circle(window.image, (x, y), 5, markcolor, marktype)

# Callback function for window
def window_mouse_callback(event, x, y, flags, param):
    window = param
    gui = window.gui
    focus = gui.focus
	
	# If this window is the focus, then accept left mouse click
    if event == cv.CV_EVENT_LBUTTONDOWN and window.side == focus:
        criteria = (cv.CV_TERMCRIT_ITER | cv.CV_TERMCRIT_EPS, 30, 0.01)
        [(fx, fy)] = cv.FindCornerSubPix(window.gray, [(x, y)], (4, 4), (0, 0), criteria)
        print "add", focus, ":", (x,y), (fx,fy)
        cv.Circle(window.image, (int(fx+0.5), int(fy+0.5)), 5, markcolor, marktype)
        cv.ShowImage(window.winname, window.image)
        gui.ptlist[focus].append((fx, fy))
        gui.focus = (focus + 1) % 2   # Switch focus
	
	# If right button down, then delete selected point
    if event == cv.CV_EVENT_RBUTTONDOWN and focus == 0:
        index, px, py = search_point(gui.ptlist[window.side], x, y, 3)
        if index >= 0:
            print "delete:", gui.ptlist[0][index], gui.ptlist[1][index]
            del gui.ptlist[0][index]
            del gui.ptlist[1][index]
            redraw_points(window)
            otherside = (window.side + 1) % 2
            redraw_points(gui.win[otherside])
            cv.ShowImage(window.winname, window.image)
            cv.ShowImage(gui.win[otherside].winname, gui.win[otherside].image)

# Window class and GUI class for organizing the windows
class Window:
    def __init__(self, gui, imagefname, side):
        self.gui = gui
        self.side = side
        self.winname = imagefname
        self.window = cv.NamedWindow(self.winname, cv.CV_WINDOW_AUTOSIZE)
        cv.SetMouseCallback(self.winname, window_mouse_callback, self)
        self.inimg = cv.LoadImage(imagefname, cv.CV_LOAD_IMAGE_COLOR)
        self.image = cv.LoadImage(imagefname, cv.CV_LOAD_IMAGE_COLOR)
        self.gray = cv.CreateMat(self.image.height, self.image.width, cv.CV_8UC1)
        cv.CvtColor(self.image, self.gray, cv.CV_RGB2GRAY)
        cv.ShowImage(self.winname, self.image)
			
class GUI:
    def __init__(self, imgfname1, imgfname2):
	    self.focus = 0
	    self.ptlist = [[], []]
	    self.win = [0, 0]
	    self.win[0] = Window(self, imgfname1, 0)
	    self.win[1] = Window(self, imgfname2, 1)
	   
    def save_points(self, ptfname1, ptfname2, markfile1, markfile2):
        file = open(ptfname1, "w")
        ptlist = self.ptlist[0]
        for i in range(len(ptlist)):
			file.write(str(ptlist[i][0]) + " " + str(ptlist[i][1]) + "\n")
        file.close()
        cv.SaveImage(markfile1, self.win[0].image)

        file = open(ptfname2, "w")
        ptlist = self.ptlist[1]
        for i in range(len(ptlist)):
			file.write(str(ptlist[i][0]) + " " + str(ptlist[i][1]) + "\n")
        file.close()
        cv.SaveImage(markfile2, self.win[1].image)
	   
		
#-----------------------------------------
# Main 

import sys
sys.path.append("C:\OpenCV2.1\Python2.6\Lib\site-packages")
import cv2
from cv2 import cv

dir = "./"
imgfile1 = dir + "image-7.jpg"
imgfile2 = dir + "image-8.jpg"
markfile1 = dir + "mark-7.jpg"
markfile2 = dir + "mark-8.jpg"
ptfile1 = dir + "imgpt-7.txt"
ptfile2 = dir + "imgpt-8.txt"
markcolor = (0, 0, 255)
marktype = 2

# Create and run GUI
gui = GUI(imgfile1, imgfile2)

while True:	
	if cv.WaitKey(0) == 27:   # Wait for 0 ms and check key press. ASCII 27 is the ESC key.
		break

gui.save_points(ptfile1, ptfile2, markfile1, markfile2)

# Clean up
cv.DestroyWindow(gui.win[0].winname)
cv.DestroyWindow(gui.win[1].winname)
