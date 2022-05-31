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



#ifndef  _SO_SF_DMBUFFER_DMIC_MOVIE_ENGINE_H
#define  _SO_SF_DMBUFFER_DMIC_MOVIE_ENGINE_H

#include <Inventor/engines/SoDMBufferEngine.h>

// System V
#include <ulocks.h>
#include "list.h"

// Digital Media
#include <dmedia/dmedia.h>
#include <movie.h>

class SoDMBufferDMICMovieEngine : public SoDMBufferEngine {

    SO_ENGINE_HEADER( SoDMBufferDMICMovieEngine );

  public:

    enum { DMIC_SEND, DMIC_RECEIVE };
    typedef struct {
	SbDMBuffer *movieDMBuffer;
	SoDMBufferDMICMovieEngine *engine;
	int type;
    } BurnItem;

    SbBool		PlayAllFrames;
    SoSFTrigger 	trigger;
    SoEngineOutput 	linearBits;
    SoEngineOutput	scale;

    static void 	initClass();
    SoDMBufferDMICMovieEngine(const char* movieFielname = NULL );

    virtual void stop( void );
    virtual void start( void );

  protected:
    virtual SbBool	openMovie( const char *filename );
    virtual void	closeMovie( void );
    virtual void	playMovie( void );
    virtual void	stopMovie( void );
    virtual void	rewindMovie( void );
    virtual size_t	movieGetNextField( unsigned char *cp );
    virtual size_t	movieGetNextFrame( void );
    virtual void	doDMBufferandMovieSetup( void );
    virtual Bool	needNextFrame( void );
  private:

    ~SoDMBufferDMICMovieEngine();

    virtual void	inputChanged(SoField *);
    virtual void 	evaluate();
    static  void 	doUpdate(void *data, SoSensor *);

    SoTimerSensor 	frameRate;
    SbDMBuffer 		*movieDMBuffer;
    SbMatrix		movieMatrix;

    // Movies
    MVtime		lastFrameIndex;
    MVid		mvid;
    MVid		mvtrackid;
    MVtime		frameindex, numFrames;
    double		rate;
    int			field;
    SbTime		startTime;
    SoTimerSensor 	*frameRateSensor;

    static pid_t child;

    static void burn( void *);
    static void registerBurnItem( SbDMBuffer *videoDMBuffer, SoDMBufferDMICMovieEngine *engine );
    static unsigned int burnCount;
    static usptr_t *arena;
    static usema_t *burnSema;
    static list<BurnItem *> burnStack;
    static struct pollfd *pollfd;
};

#endif
