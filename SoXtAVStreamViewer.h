#ifndef SOXTAVSTREAMVIEWER_H_
#define SOXTAVSTREAMVIEWER_H_

#include <Inventor/Xt/viewer/SoXtFullViewer.h>
#include <Inventor/SbLinear.h>

class SoXtAVStreamViewer : public SoXtFullViewer {
	public:
		SoXtAVStreamViewer(
			Widget parent = NULL,
			const char *name = NULL,
			SbBool buildInsideParent = TRUE,
			SoXtFullViewer::BuildFlag flag = BUILD_ALL,
			SoXtViewer::Type type = BROWSER);
		~SoXtAVStreamViewer();

		virtual void setViewing(SbBool onOrOff);

	protected:
		virtual void processEvent(XAnyEvent *anyevent);
		virtual void setSeekMode(SbBool onOrOff);
		virtual void bottomWheelMotion(float newVal);
		virtual void leftWheelMotion(float newVal);
		virtual void rightWheelMotion(float newVal);
		virtual void bottomWheelStart();
		virtual void leftWheelStart();
		virtual void createPrefSheet();
		virtual void openViewerHelpCard();

	private:
		int mode;
		SbBool createdCursors;
		Cursor vwrCursor;
		SbVec2s locator;
		SbVec3f locator3D;
		SbPlane focalplane;
		float transXspeed, transYspeed;

		void switchMode(int newMode);
		void defineCursors();
		void translateCamera();
		void computeTranslateValues();
};

#endif
