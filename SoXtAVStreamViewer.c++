#include <math.h>
#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/Xt/SoXtCursors.h>
#include "SoXtAVStreamViewer.h"

enum ViewerModes {
	IDLE_MODE,
	TRANS_MODE,
	SEEK_MODE,
};

SoXtAVStreamViewer::SoXtAVStreamViewer(
		Widget parent,
		const char *name,
		SbBool buildInsideParent,
		SoXtFullViewer::BuildFlag b,
		SoXtViewer::Type t)
	: SoXtFullViewer(
			parent,
			name,
			buildInsideParent,
			b,
			t,
			TRUE)
{
	mode = IDLE_MODE;
	createdCursors = FALSE;
	setSize(SbVec2s(520, 360));

	setPopupMenuString("Simple Viewer");
	setupBottomWheelString("transX");
	setupLeftWheelString("transY");
	setupRightWheelString("Dolly");
	setPrefSheetString("AVStream Viewer Preference Sheet");
	setTitle("AVStream Viewer");
}

SoXtAVStreamViewer::~SoXtAVStreamViewer() {}

void SoXtAVStreamViewer::setViewing(SbBool flag) {
	if(flag == viewingFlag || camera == NULL) {
		viewingFlag = flag;
		return;
	}

	SoXtFullViewer::setViewing(flag);

	Widget w = getRenderAreaWidget();
	if (w != NULL && XtWindow(w) != NULL) {
		if(isViewing()) {
			if(!createdCursors)
				defineCursors();
			XDefineCursor(XtDisplay(w), XtWindow(w), vwrCursor);
		}
		else
			XtUndefineCursor(XtDisplay(w), XtWindow(w));
	}
}

void SoXtAVStreamViewer::processEvent(XAnyEvent *xe) {
	if(processCommonEvents(xe))
		return;

	if(!createdCursors) {
		defineCursors();
		Widget w = getRenderAreaWidget();
		XDefineCursor(XtDisplay(w), XtWindow(w), vwrCursor);
	}

	XButtonEvent *be;
	XMotionEvent *me;
	SbVec2s windowSize = getGlxSize();

	switch(xe->type) {
		case ButtonPress:
			be = (XButtonEvent *)xe;
			locator[0] = be->x;
			locator[1] = windowSize[1] - be->y;
			if(be->button == Button1) {
				switch(mode) {
					case IDLE_MODE:
						interactiveCountInc();
						switchMode(TRANS_MODE);
						break;
					case SEEK_MODE:
						seekToPoint(locator);
						break;
				}
			}
			break;
		case ButtonRelease:
			be = (XButtonEvent *)xe;
			if(be->button == Button1 && mode == TRANS_MODE) {
				switchMode(IDLE_MODE);
				interactiveCountDec();
			}
			break;
		case MotionNotify:
			me = (XMotionEvent *)xe;
			locator[0] = me->x;
			locator[1] = windowSize[1] - me->y;
			if(mode == TRANS_MODE)
				translateCamera();
			break;
	}
}

void SoXtAVStreamViewer::switchMode(int newMode) {
	Widget w = getRenderAreaWidget();
	Display *display = XtDisplay(w);
	Window window = XtWindow(w);
	if(!createdCursors)
		defineCursors();

	mode = newMode;
	switch)mode) {
		case IDLE_MODE:
			if(window != 0)
				XDefineCursor(display, window, vwrCursor);
			break;
		case TRANS_MODE:
			{
				SbMatrix mx = camera->orientation.getValue();
				SbVec3f forward(-mx[2][0], -mx[2][1], -mx[2][2]);
				SbVec3f fp = camera->position.getValue() +
					forward * camera->focalDistance.getValue();
				focalPlane = SbPlane(forward, fp);

				SbVec2s windowSize = getGlxSize();
				SbLine line;
				SbViewVolume cameraVolume = camera->getViewVolume();
				cameraVolume.projectPointToLine(
						SbVec2f(locator[0]) / float(windowSize[0],
							locator[1] / float(windowSize[1])), line);
				focalPlane.intersect(line, locator3D);
			}
			if(window != 0)
				XDefineCursor(display, window, vwrCursor);
			break;
		case SEEK_MODE:
			if(window != 0)
				XDefineCursor(display, window, seekCursor);
			break;
	}
}
