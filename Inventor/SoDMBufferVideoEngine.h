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

#ifndef  _SO_SF_DMBUFFER_VIDEO_ENGINE_H
#define  _SO_SF_DMBUFFER_VIDEO_ENGINE_H

#include <Inventor/engines/SoDMBufferEngine.h>

// System V
#include <ulocks.h>
#include "list.h"

class SoDMBufferVideoEngine : public SoDMBufferEngine {

    SO_ENGINE_HEADER( SoDMBufferVideoEngine );

  public:

#ifndef INSTANCE_THREADING
    typedef struct {
	SbDMBuffer *videoDMBuffer;
	VLPath path;
    } BurnItem;
#endif

    const static short VIDEO_WIDTH  = 512;
    const static short VIDEO_HEIGHT = 512;
    const static short VIDEO_BUFFER_COUNT =   5;

    SoSFTrigger trigger;
    SoEngineOutput textureBits;
    SoEngineOutput linearBits;
    SoEngineOutput scale;

    enum { LAYOUT_GRAPHICS,
	   LAYOUT_LINEAR,
	   LAYOUT_BOTH };

    static void initClass();
    SoDMBufferVideoEngine( int layout = LAYOUT_GRAPHICS,
			   const char* videoSource = NULL, 
			   int originX = 0,
			   int originY = 32,
			   int zoomNumerator = -1,
			   int zoomDenominator = -1,
			   int aspectNumerator = -1,
			   int aspectDenominator = -1,
			   int width = VIDEO_WIDTH, 
			   int height = VIDEO_HEIGHT,
			   int bufferCount = VIDEO_BUFFER_COUNT );

    virtual void stop( void );
    virtual void start( void );

  private:

    ~SoDMBufferVideoEngine();

    virtual void inputChanged(SoField *);
    virtual void evaluate();
#ifdef INSTANCE_THREADING
    static  void burnCB( void *data );
    virtual void burn( void );

    pid_t child;
    VLServer server, linearServer;
#else
    static pid_t child;
    static VLServer server;

    static void burn( void *);
    static void registerBurnItem( SbDMBuffer *videoDMBuffer, VLPath path );
    static unsigned int burnCount;
    static usptr_t *arena;
    static usema_t *burnSema;
    static list<BurnItem *> burnStack;
    static struct pollfd *pollfd;
#endif
    static  void doUpdate(void *data, SoSensor *);

    SoTimerSensor *frameRateSensor;

    SbMatrix	 videoMatrix;

    SoTimerSensor frameRate;
    SbDMBuffer *videoDMBuffer, *videoDMBufferLinear;
    VLPath path, linearPath;


};

#endif
