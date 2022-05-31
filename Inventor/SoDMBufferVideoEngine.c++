 /****************************************************************************
 *                                                                          *
 *               Copyright (c) 1996-97-98, Silicon Graphics, Inc.           *
 *                                                                          *
 *   Permission to use,  copy, modify, distribute, and sell this software   *
 *   and its documentation for any purpose is hereby granted without fee,   *
 *   provided that the name of  Silicon  Graphics  may not be used in any   *
 *   advertising  or  publicity  relating  to  the  software  without the   *
 *   specific, prior written permission of Silicon Graphics.                *
 *                                                                          *
 *   THIS SOFTWARE IS PROVIDED "AS-IS"  AND WITHOUT WARRANTY OF ANY KIND,   *
 *   EXPRESS,  IMPLIED OR OTHERWISE,  INCLUDING  WITHOUT LIMITATION,  ANY   *
 *   WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.       *
 *                                                                          *
 *   IN  NO  EVENT  SHALL  SILICON  GRAPHICS  BE  LIABLE FOR ANY SPECIAL,   *
 *   INCIDENTAL,  INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,  OR  ANY   *
 *   DAMAGES  WHATSOEVER  RESULTING  FROM  LOSS OF USE,  DATA OR PROFITS,   *
 *   WHETHER OR NOT ADVISED OF  THE  POSSIBILITY  OF  DAMAGE,  AND ON ANY   *
 *   THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR   *
 *   PERFORMANCE OF THIS SOFTWARE.                                          *
 *                                                                          *
 ****************************************************************************/



 
#include <Inventor/fields/SoSFMatrix.h>
#include <Inventor/engines/SoDMBufferVideoEngine.h>

// System V
#include <stropts.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/prctl.h>

// Local
#include <Inventor/misc/checks.h>

SO_ENGINE_SOURCE( SoDMBufferVideoEngine );

void
SoDMBufferVideoEngine::initClass( void ){
    SO_ENGINE_INIT_CLASS( SoDMBufferVideoEngine, SoEngine, "Engine");

#ifndef INSTANCE_THREADING
    //--- Semaphores -----------------------------------
    usconfig(CONF_ARENATYPE, US_SHAREDONLY);
    arena = usinit(mktemp("/tmp/alXXXXXX"));
    if (arena == NULL)
      perror("Trouble creating arena.\n");

    burnSema = usnewsema(arena, 1);
    if (burnSema == NULL)
	perror("SbDMBuffer::Trouble creating mutex semaphore.\n");

    //--- Semaphores  -----------------------------------

    //--- Video Thread Creation -----------------------------------
    prctl(PR_SETEXITSIG, SIGTERM);

    if ((child = sproc(burn, PR_SALL, NULL)) == -1)
	fprintf(stderr, "Form::open unable to thread video.\n");
    //--- Video Thread Creation -----------------------------------

    XCHECK_VL( server = vlOpenVideo(NULL), "Unable to open video device\n");

#endif // INSTANCE_THREADING
}
    
SoDMBufferVideoEngine::SoDMBufferVideoEngine( int layout,
					      const char* videoSource, 
					      int originX,
					      int originY,
					      int zoomNumerator,
					      int zoomDenominator,
					      int aspectNumerator,
					      int aspectDenominator,
					      int width, 
					      int height,
					      int bufferCount)
{
    SO_ENGINE_CONSTRUCTOR( SoDMBufferVideoEngine );

    SO_ENGINE_ADD_INPUT( trigger, () );
    SO_ENGINE_ADD_OUTPUT( textureBits, SoSFDMBufferImage );
    SO_ENGINE_ADD_OUTPUT( linearBits, SoSFDMBufferImage );
    SO_ENGINE_ADD_OUTPUT( scale, SoSFMatrix );

    Display *display = NULL;
    VLNode drain;

    linearPath = NULL;
    path = NULL;

    if ( layout == LAYOUT_GRAPHICS ||
	 layout == LAYOUT_BOTH ){
	videoDMBuffer = new SbDMBuffer( SbVec2s( width, height) );

	float rx = (float) (width-originX)/(float) width;
	float ry = (float) (height-originY)/(float) height;

	float scale[4][4] = { rx,  0,  0,  0,
			      0, ry,  0,  0,
			      0,  0,  1,  0,
			      0,  0,  0,  1 };

	videoMatrix.setValue( scale );

	openStandardVideo( videoDMBuffer, videoSource, SbVec2s( originX, originY ),
			   SbVec2s( zoomNumerator, zoomDenominator ),
			   SbVec2s( aspectNumerator, aspectDenominator ),
			   server, path, drain, VL_LAYOUT_GRAPHICS );
	openStandardPBuffer( display, videoDMBuffer );
	videoDMBuffer->setBufferCount( bufferCount );
	videoDMBuffer->createPool();
	videoDMBuffer->manage( server, path, drain);

	startVideo( server, path );
    }

    if ( layout == LAYOUT_LINEAR ||
	 layout == LAYOUT_BOTH ) {
	// Linear
	videoDMBufferLinear = new SbDMBuffer( SbVec2s( width, height) );
	openStandardVideo( videoDMBufferLinear, videoSource, 
			   SbVec2s( originX, originY),
			   SbVec2s( zoomNumerator, zoomDenominator ),
			   SbVec2s( aspectNumerator, aspectDenominator ),
			   server, linearPath, drain, VL_LAYOUT_LINEAR );
	videoDMBufferLinear->setBufferCount( bufferCount );
	videoDMBufferLinear->createPool();
	videoDMBufferLinear->manage( server, linearPath, drain);

	startVideo( server, linearPath );
    }
#ifdef INSTANCE_THREADING
    //--- Video Thread Creation -----------------------------------
    prctl(PR_SETEXITSIG, SIGTERM);

    if ((child = sproc(burnCB, PR_SALL, this)) == -1)
	fprintf(stderr, "Form::open unable to thread video.\n");
    //--- Video Thread Creation -----------------------------------
#else // INSTANCE_THREADING
    if ( layout == LAYOUT_GRAPHICS ||
	 layout == LAYOUT_BOTH )
	registerBurnItem( videoDMBuffer, path );
    if ( layout == LAYOUT_LINEAR ||
	 layout == LAYOUT_BOTH )
	registerBurnItem( videoDMBufferLinear, linearPath );
#endif

     frameRateSensor = new SoTimerSensor(doUpdate, this);
     frameRateSensor->setInterval(1.0/30.0);
     frameRateSensor->schedule();
}

SoDMBufferVideoEngine::~SoDMBufferVideoEngine(){
    kill(child,SIGTERM);
    delete videoDMBuffer;
    vlEndTransfer(server, path);
    vlDestroyPath(server, path);

    delete videoDMBufferLinear;
    vlEndTransfer(server, linearPath);
    vlDestroyPath(server, linearPath);
}

void
SoDMBufferVideoEngine::inputChanged(SoField *){
    evaluate();
}

void
SoDMBufferVideoEngine::stop(){
    frameRateSensor->unschedule();
    if ( linearPath != NULL )
	stopVideo( server, linearPath );
    if ( path != NULL )
	stopVideo( server, path );
}

void
SoDMBufferVideoEngine::start(){
    frameRateSensor->schedule();
    if ( linearPath != NULL )
	startVideo( server, linearPath );
    if ( path != NULL )
	startVideo( server, path );
}

void
SoDMBufferVideoEngine::evaluate(){
    trigger.getValue();
    SO_ENGINE_OUTPUT( scale, SoSFMatrix, setValue( videoMatrix ));
    if ( path != NULL )
	SO_ENGINE_OUTPUT( textureBits, SoSFDMBufferImage, setValue( videoDMBuffer ));
    if ( linearPath != NULL )
	SO_ENGINE_OUTPUT( linearBits, SoSFDMBufferImage, setValue( videoDMBufferLinear ));
}

void
SoDMBufferVideoEngine::doUpdate(void *data, SoSensor *){
    SoDMBufferVideoEngine *engine = (SoDMBufferVideoEngine *) data;
    engine->trigger.setValue();
}

#ifdef INSTANCE_THREADING
void
SoDMBufferVideoEngine::burnCB( void *data ){
    SoDMBufferVideoEngine *engine = (SoDMBufferVideoEngine *) data;
    engine->burn();
}

void
SoDMBufferVideoEngine::burn( void ){

    //--- Setup Video Path Polling Parameters -----------------------------------------
    struct pollfd pollfd[2];

    XCHECK( ( vlPathGetFD(server, path, &pollfd[0].fd) == 0 ),
	    "Failed to get video file descriptor.\n" );
    XCHECK( ( vlPathGetFD(server, linearPath, &pollfd[1].fd) == 0 ),
	    "Failed to get video file descriptor.\n" );

    pollfd[0].events = POLLIN;
    pollfd[1].events = POLLIN;

    //--- Polling Loop ----------------------------------------------------------------
    VLEvent event;
    for(;;)
	if ( poll(pollfd, 2, INFTIM) < 0 )
	    fprintf(stderr , "Error polling video...\n");
	else{

	    if ( pollfd[0].revents == POLLIN &&
		 vlEventRecv(server, path, &event) != -1 &&
		 event.reason == VLTransferComplete )
		    videoDMBuffer->setValueFromVLEvent(&event);

	    if ( pollfd[1].revents == POLLIN &&
		 vlEventRecv(server, linearPath, &event) != -1 &&
		 event.reason == VLTransferComplete )
		    videoDMBufferLinear->setValueFromVLEvent(&event);

#ifdef DEBUG
	    printf("=== %d \n", event.reason);
	    if ( event.reason == VLStreamBusy )
		printf("\t VLStreamBusy \n");
	    if ( event.reason == VLStreamPreempted )
		printf("\t VLStreamPreempted \n");
	    if ( event.reason == VLAdvanceMissed )
		printf("\t VLAdvanceMissed \n");
	    if ( event.reason == VLStreamAvailable )
		printf("\t VLStreamAvailable \n");
	    if ( event.reason == VLSyncLost )
		printf("\t VLSyncLost \n");
	    if ( event.reason == VLStreamStarted )
		printf("\t VLStreamStarted \n");
	    if ( event.reason == VLStreamStopped )
		printf("\t VLStreamStopped \n");
	    if ( event.reason == VLSequenceLost )
		printf("\t VLSequenceLost \n");
	    if ( event.reason == VLControlChanged )
		printf("\t VLControlChanged \n");
	    if ( event.reason == VLTransferComplete )
		printf("\t VLTransferComplete \n");
	    if ( event.reason == VLTransferFailed )
		printf("\t VLTransferFailed \n");
	    if ( event.reason == VLEvenVerticalRetrace )
		printf("\t VLEvenVerticalRetrace \n");
	    if ( event.reason == VLOddVerticalRetrace )
		printf("\t VLOddVerticalRetrace \n");
	    if ( event.reason == VLFrameVerticalRetrace )
		printf("\t VLFrameVerticalRetrace \n");
	    if ( event.reason == VLDeviceEvent )
		printf("\t VLDeviceEvent \n");
	    if ( event.reason == VLDefaultSource )
		printf("\t VLDefaultSource \n");
	    if ( event.reason == VLControlRangeChanged )
		printf("\t VLControlRangeChanged \n");
	    if ( event.reason == VLControlPreempted )
		printf("\t VLControlPreempted \n");
	    if ( event.reason == VLControlAvailable )
		printf("\t VLControlAvailable \n");
	    if ( event.reason == VLDefaultDrain )
		printf("\t VLDefaultDrain \n");
	    if ( event.reason == VLStreamChanged )
		printf("\t VLStreamChanged \n");
	    if ( event.reason == VLTransferError )
		printf("\t VLTransferError \n");
#endif
	}
}

#else // INSTANCE_THREADING

unsigned int SoDMBufferVideoEngine::burnCount = 0;
usptr_t *SoDMBufferVideoEngine::arena = NULL;
usema_t *SoDMBufferVideoEngine::burnSema = NULL;
pid_t SoDMBufferVideoEngine::child = NULL;
struct pollfd *SoDMBufferVideoEngine::pollfd = NULL;
list<SoDMBufferVideoEngine::BurnItem *> SoDMBufferVideoEngine::burnStack;
VLServer SoDMBufferVideoEngine::server;

void
SoDMBufferVideoEngine::registerBurnItem( SbDMBuffer *videoDMBuffer, VLPath path ){

    BurnItem *bi = new BurnItem;
    bi->videoDMBuffer = videoDMBuffer;
    bi->path = path;

    uspsema(burnSema);

    burnStack.push_back( bi );
    burnCount ++;

    if ( pollfd != NULL )
	delete [] pollfd;
    pollfd = new struct pollfd[burnCount];

    int j = 0;
    list<BurnItem *>::iterator i;
    for(i=burnStack.begin();i!=burnStack.end();i++){
	XCHECK( ( vlPathGetFD(server, (*i)->path, &pollfd[j].fd) == 0 ),
		"Failed to get video file descriptor.\n" );
	pollfd[j].events = POLLIN;
	j++;
    }
    usvsema(burnSema);
}

void
SoDMBufferVideoEngine::burn( void *){

    //--- Polling Loop ----------------------------------------------------------------
    VLEvent event;
    for(;;){
	uspsema(burnSema);
	if ( burnCount == 0 ){
	    usvsema(burnSema);
	    continue;
	}

	if ( poll(pollfd, burnCount, INFTIM) < 0 )
	    fprintf(stderr , "Error polling video...\n");
	else{

	    int j = 0;
	    list<SoDMBufferVideoEngine::BurnItem *>::iterator i;
	    for(i=burnStack.begin();i!=burnStack.end();i++){
		if ( pollfd[j].revents == POLLIN &&
		     vlEventRecv(server, (*i)->path, &event) != -1 &&
		     event.reason == VLTransferComplete )
		    (*i)->videoDMBuffer->setValueFromVLEvent(&event);
		j++;
	    }
	}
	usvsema(burnSema);
    }
}
#endif // INSTANCE_THREADING


