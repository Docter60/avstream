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
	setBottomWheelString("transX");
	setLeftWheelString("transY");
	setRightWheelString("Dolly");
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
			XUndefineCursor(XtDisplay(w), XtWindow(w));
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
	switch(mode) {
		case IDLE_MODE:
			if(window != 0)
				XDefineCursor(display, window, vwrCursor);
			break;
		case TRANS_MODE:
			{
				SbMatrix mx;
				mx = camera->orientation.getValue();
				SbVec3f forward(-mx[2][0], -mx[2][1], -mx[2][2]);
				SbVec3f fp = camera->position.getValue() +
					forward * camera->focalDistance.getValue();
				focalplane = SbPlane(forward, fp);

				SbVec2s windowSize = getGlxSize();
				SbLine line;
				SbViewVolume cameraVolume = camera->getViewVolume();
				cameraVolume.projectPointToLine(
						SbVec2f(locator[0] / float(windowSize[0]),
							locator[1] / float(windowSize[1])), line);
				focalplane.intersect(line, locator3D);
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

void SoXtAVStreamViewer::setSeekMode(SbBool flag) {
	if(!isViewing())
		return;
	SoXtFullViewer::setSeekMode(flag);
	switchMode(isSeekMode() ? SEEK_MODE : IDLE_MODE);
}

void SoXtAVStreamViewer::createPrefSheet() {
	Widget shell, form;
	createPrefSheetShellAndForm(shell, form);

	Widget widgetList[10];
	int num = 0;
	widgetList[num++] = createSeekPrefSheetGuts(form);
	widgetList[num++] = createZoomPrefSheetGuts(form);
	widgetList[num++] = createClippingPrefSheetGuts(form);

	layoutPartsAndMapPrefSheet(widgetList, num, form, shell);
}

void SoXtAVStreamViewer::openViewerHelpCard() {
	openHelpCard("soXtAVStreamViewer.help");
}

void SoXtAVStreamViewer::bottomWheelMotion(float newVal) {
	if(camera == NULL)
		return;

		SbMatrix mx;
		mx = camera->orientation.getValue();
		SbVec3f rightVector(mx[0][0], mx[0][1], mx[0][2]);
		float dist = transXspeed * (bottomWheelVal - newVal);
		camera->position = camera->position.getValue() +
			dist * rightVector;

		bottomWheelVal = newVal;
}

void SoXtAVStreamViewer::leftWheelMotion(float newVal) {
	if(camera == NULL)
		return;
	
	SbMatrix mx;
	mx = camera->orientation.getValue();
	SbVec3f upVector(mx[1][0], mx[1][1], mx[1][2]);
	float dist = transYspeed * (leftWheelVal - newVal);
	camera->position = camera->position.getValue() +
		dist * upVector;
	
	leftWheelVal = newVal;
}

void SoXtAVStreamViewer::rightWheelMotion(float newVal) {
	if(camera == NULL)
		return;
	
	float focalDistance = camera->focalDistance.getValue();
	float newFocalDistance = focalDistance / pow(2.0f, newVal - rightWheelVal);
	
	SbMatrix mx;
	mx = camera->orientation.getValue();
	SbVec3f forward(-mx[2][0], -mx[2][1], -mx[2][2]);
	camera->position = camera->position.getValue() +
		(focalDistance - newFocalDistance) * forward;
	camera->focalDistance = newFocalDistance;
	
	rightWheelVal = newVal;
}

void SoXtAVStreamViewer::defineCursors() {
	XColor foreground;
	Pixmap source;
	Display *display = getDisplay();
	Drawable d = DefaultRootWindow(display);

	foreground.red = 65535;
	foreground.green = foreground.blue = 0;

	source = XCreateBitmapFromData(display, d,
		so_xt_flat_hand_bits, so_xt_flat_hand_width,
		so_xt_flat_hand_height);
	vwrCursor = XCreatePixmapCursor(display, source, source,
		&foreground, &foreground, so_xt_flat_hand_x_hot,
		so_xt_flat_hand_y_hot);
	XFreePixmap(display, source);

	source = XCreateBitmapFromData(display, d,
		so_xt_target_bits, so_xt_target_width,
		so_xt_target_height);
	seekCursor = XCreatePixmapCursor(display, source, source,
		&foreground, &foreground, so_xt_target_x_hot,
		so_xt_target_y_hot);
	XFreePixmap(display, source);

	createdCursors = TRUE;
}

void SoXtAVStreamViewer::translateCamera() {
	if(camera == NULL)
		return;
	
	SbVec2s windowSize = getGlxSize();
	SbVec2f newLocator(locator[0] / float(windowSize[0]),
		locator[1] / float(windowSize[1]));
	
	SbLine line;
	SbVec3f newLocator3D;
	SbViewVolume cameraVolume = camera->getViewVolume();
	cameraVolume.projectPointToLine(newLocator, line);
	focalplane.intersect(line, newLocator3D);

	camera->position = camera->position.getValue() +
		(locator3D - newLocator3D);
}

void SoXtAVStreamViewer::computeTranslateValues() {
	if(camera == NULL)
		return;
	
	float height;

	if(camera->isOfType(
		SoPerspectiveCamera::getClassTypeId())) {
		float angle = ((SoPerspectiveCamera *)camera)->heightAngle.getValue() / 2;
		float dist = camera->focalDistance.getValue();
		height = dist * ftan(angle);
	}
	else if(camera->isOfType(
		SoOrthographicCamera::getClassTypeId())) {
		height = ((SoOrthographicCamera *)camera)->height.getValue() / 2;
	}

	transXspeed = height / 2;
	transYspeed = transYspeed * camera->aspectRatio.getValue();
}

void SoXtAVStreamViewer::bottomWheelStart() {
	computeTranslateValues();
	SoXtFullViewer::bottomWheelStart();
}

void SoXtAVStreamViewer::leftWheelStart() {
	computeTranslateValues();
	SoXtFullViewer::leftWheelStart();
}
