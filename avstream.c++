#include <dmedia/audio.h>
#include <dmedia/dmedia.h>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoDMBufferTexture2.h>
#include <Inventor/engines/SoDMBufferVideoEngine.h>
#include <Inventor/Xt/SoXt.h>
#include <Inventor/Xt/viewers/SoXtExaminerViewer.h>
#include <Xm/Protocols.h>
#include <Xm/Label.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <unistd.h>

static int audioConnectionID = 0;

void quitFunc(Widget, XtPointer, XtPointer);

int main(int , char **argv) {
   Widget myWindow = SoXt::init(argv[0]);
   if(myWindow == NULL) exit(-1);

   // Set up window quit and close callbacks
   Atom WM_QUIT_APP = XmInternAtom(XtDisplay(myWindow), "_WM_QUIT_APP", FALSE);
   Atom WM_CLOSE_APP = XmInternAtom(XtDisplay(myWindow), "WM_DELETE_WINDOW", FALSE);
   XmAddWMProtocolCallback(myWindow, WM_QUIT_APP, quitFunc, NULL);
   XmAddWMProtocolCallback(myWindow, WM_CLOSE_APP, quitFunc, NULL);

   // Must be called after SoXt::init for proper registration
   SoDMBufferTexture2::initClass();
   SoDMBufferVideoEngine::initClass();

   SoSeparator *root = new SoSeparator;
   root->ref();

   // Setup composite stream texture
   SoDMBufferTexture2 *texture = new SoDMBufferTexture2;
   root->addChild(texture);
   SoDMBufferVideoEngine *videoIn = new SoDMBufferVideoEngine;
   texture->image.connectFrom(&videoIn->textureBits);
   texture->transform.connectFrom(&videoIn->scale);

   root->addChild(new SoCube);

   SoXtExaminerViewer *myViewer = 
            new SoXtExaminerViewer(myWindow);
   myViewer->setSceneGraph(root);
   myViewer->setTitle("Composite In Streaming");

   // In Inventor 2.1, if the machine does not have hardware texture
   // mapping, we must override the default drawStyle to display textures.
   myViewer->setDrawStyle(SoXtViewer::STILL, SoXtViewer::VIEW_AS_IS);

   myViewer->show();

   SoXt::show(myWindow);

   ALpv pv;
   int in = alGetResourceByName(AL_SYSTEM, "AnalogIn", AL_DEVICE_TYPE);
   int line = alGetResourceByName(AL_SYSTEM, "LineIn", AL_INTERFACE_TYPE);
   pv.param = AL_INTERFACE;
   pv.value.i = line;
   if(alSetParams(in, &pv, 1) < 0 || pv.sizeOut < 0) {
      printf("set interface failed.\n");
   }
   //printf("line: %d\n", line);
   audioConnectionID = alConnect(AL_DEFAULT_INPUT, AL_DEFAULT_OUTPUT, NULL, 0);

   SoXt::mainLoop();
   return 0;
}

void quitFunc(Widget w, XtPointer clientData, XtPointer cbp) {
   int ret = alDisconnect(audioConnectionID);
   if(ret == -1) {
      switch(oserror()) {
         case AL_BAD_RESOURCE:
            printf("Bad resource. Doesn't exist.\n");
            break;
         case AL_BAD_DEVICE_ACCESS:
            printf("Bad device access. Audio system not present or improper.\n");
            break;
         default: break;
      }
   }
   exit(0);
}
