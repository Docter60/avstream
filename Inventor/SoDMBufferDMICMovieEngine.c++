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
#include <Inventor/engines/SoDMBufferDMICMovieEngine.h>

// System V
#include <stropts.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/prctl.h>

// Local
#include <Inventor/misc/checks.h>

SO_ENGINE_SOURCE( SoDMBufferDMICMovieEngine );

unsigned int SoDMBufferDMICMovieEngine::burnCount = 0;
usptr_t *SoDMBufferDMICMovieEngine::arena = NULL;
usema_t *SoDMBufferDMICMovieEngine::burnSema = NULL;
pid_t SoDMBufferDMICMovieEngine::child = NULL;
struct pollfd *SoDMBufferDMICMovieEngine::pollfd = NULL;
list<SoDMBufferDMICMovieEngine::BurnItem *> SoDMBufferDMICMovieEngine::burnStack;

void
SoDMBufferDMICMovieEngine::initClass( void ){
    SO_ENGINE_INIT_CLASS( SoDMBufferDMICMovieEngine, SoEngine, "Engine");

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

}

SoDMBufferDMICMovieEngine::SoDMBufferDMICMovieEngine(const char* movieFilename )
{
    SO_ENGINE_CONSTRUCTOR( SoDMBufferDMICMovieEngine );

    SO_ENGINE_ADD_INPUT( trigger, () );
    SO_ENGINE_ADD_OUTPUT( linearBits, SoSFDMBufferImage );
    SO_ENGINE_ADD_OUTPUT( scale, SoSFMatrix );

    lastFrameIndex = 0;
    openMovie( movieFilename );

    registerBurnItem( movieDMBuffer, this );

    PlayAllFrames = False;
}

SoDMBufferDMICMovieEngine::~SoDMBufferDMICMovieEngine(){
}

void
SoDMBufferDMICMovieEngine::inputChanged(SoField *){
    evaluate();
}

void
SoDMBufferDMICMovieEngine::evaluate(){
    trigger.getValue();
    SO_ENGINE_OUTPUT( scale, SoSFMatrix, setValue( movieMatrix ));
    SO_ENGINE_OUTPUT( linearBits, SoSFDMBufferImage, setValue( movieDMBuffer ));
}

void
SoDMBufferDMICMovieEngine::stop(){
}

void
SoDMBufferDMICMovieEngine::start(){
}

SbBool
SoDMBufferDMICMovieEngine::openMovie( const char *filename ){
  DMstatus status;
  
  XCHECK( filename != NULL, "No movie filename specified to engine.\n");
  if ( filename == NULL )
      return False;

  mvid = NULL;
  status = mvOpenFile( filename, O_RDONLY, &mvid );
  CHECK_DM( status == DM_SUCCESS, "Error opening movie.\n");

  if ( status != DM_SUCCESS){
      closeMovie();
      return False;
  }

  status = mvFindTrackByMedium( mvid, DM_IMAGE, &mvtrackid );
  CHECK_DM( status == DM_SUCCESS, "Error locating movie track.\n");
  if ( status != DM_SUCCESS){
      closeMovie();
      return False;
  }

  rate = mvGetImageRate(mvtrackid);
  numFrames = mvGetTrackLength(mvtrackid) - 1;
  field = 0;

  doDMBufferandMovieSetup();
  playMovie();

  return True;
}

void
SoDMBufferDMICMovieEngine::closeMovie( void ){
  DMstatus status = DM_SUCCESS;
  
  if ( mvid != NULL){
      status = mvClose(mvid);
      CHECK_DM( status == DM_SUCCESS, "closeMovie() Error closing movie.\n");
  }

  mvid = NULL;
}

void
SoDMBufferDMICMovieEngine::playMovie( void ){
  frameindex = 0;
  startTime = SbTime::getTimeOfDay();
}

void
SoDMBufferDMICMovieEngine::stopMovie( void ){
}

void
SoDMBufferDMICMovieEngine::rewindMovie( void ){
}

static uchar_t *
j_find(unsigned char *cp, unsigned char marker)
{
    for (;;cp++) {
	if (*cp == 0xff) {
	    if (*(cp + 1) == marker) {
		return cp;
	    }
	}
    }
}

#define	FRAMEBYTES	0x1000000 /* 1mb per 2 fields ought to be enough */
static unsigned char buf[FRAMEBYTES];

size_t
SoDMBufferDMICMovieEngine::movieGetNextField( unsigned char *cp ){
    static unsigned char *eoi;
    unsigned char *sos;
    size_t nbytes;

    if (field == 0) {
	size_t nbytes = movieGetNextFrame();
	eoi = j_find(buf, 0xd9);
	nbytes = eoi - buf + 2;
	bcopy(buf, cp, nbytes); /* XXX hmmm */
	field = 1;
	return nbytes;
    }
    /* field 1 */
    sos = j_find(eoi, 0xd8);
    eoi = j_find(sos, 0xd9);
    nbytes = eoi - sos + 2;
    bcopy(sos, cp, nbytes);
    field = 0;
    frameindex++;
    return nbytes;
}

Bool
SoDMBufferDMICMovieEngine::needNextFrame( void ){

    if (PlayAllFrames)
	frameindex++;
    else{
	SbTime currentTime = SbTime::getTimeOfDay();
	SbTime elapsedTime = (currentTime - startTime);
	frameindex = elapsedTime.getValue() * mvGetImageRate(mvtrackid);
    }

    if (frameindex >= numFrames-1){
	frameindex = 0;
	startTime = SbTime::getTimeOfDay();
    }

    if ( lastFrameIndex == frameindex )
	return False;

    lastFrameIndex = frameindex;

    return True;
}

size_t
SoDMBufferDMICMovieEngine::movieGetNextFrame( void ){
    DMstatus status;

    size_t nbytes;

    MVframe fr;
    int index;
    status = mvGetTrackDataIndexAtTime(mvtrackid, frameindex, rate, &index, &fr);
    CHECK_DM( status == DM_SUCCESS, "Error locating movie index.\n");

    MVdatatype dt;
    int paramsid;
    status = mvGetTrackDataInfo(mvtrackid, index, &nbytes, &paramsid, &dt, &fr);
    CHECK_DM( status == DM_SUCCESS, "Error locating movie data.\n");

    status = mvReadTrackData(mvtrackid, index, nbytes, buf);
    CHECK_DM( status == DM_SUCCESS, "Error reading movie data.\n");

    return nbytes;
}

void
SoDMBufferDMICMovieEngine::doDMBufferandMovieSetup( void ){

    int MOVIE_WIDTH = mvGetImageWidth(mvtrackid);
    int MOVIE_HEIGHT = mvGetImageHeight(mvtrackid);

    // Make power of 2 (larger)
    int TEX_MOVIE_WIDTH  = (int) pow(2.0f,(int) ceil(logf(MOVIE_WIDTH)/logf(2.0f)));
    int TEX_MOVIE_HEIGHT = (int) pow(2.0f,(int) ceil(logf(MOVIE_HEIGHT)/logf(2.0f)));

    float rx = (float) MOVIE_WIDTH/(float) TEX_MOVIE_WIDTH;
    float ry = (float) MOVIE_HEIGHT/(float) TEX_MOVIE_HEIGHT;

    float scale[4][4] = { rx,  0,  0,  0,
			   0, ry,  0,  0,
			   0,  0,  1,  0,
                           0,  0,  0,  1 };

    movieMatrix.setValue( scale );

    movieDMBuffer = new SbDMBuffer( SbVec2s( TEX_MOVIE_WIDTH, TEX_MOVIE_HEIGHT) );

    openStandardDMICOutputPBuffer( movieDMBuffer, SbVec2s( MOVIE_WIDTH, MOVIE_HEIGHT));


}

void
SoDMBufferDMICMovieEngine::registerBurnItem( SbDMBuffer *movieDMBuffer,
					     SoDMBufferDMICMovieEngine *engine ){

    BurnItem *bisend = new BurnItem;
    bisend->movieDMBuffer = movieDMBuffer;
    bisend->engine = engine;
    bisend->type = DMIC_SEND;
    
    BurnItem *birecv = new BurnItem;
    birecv->movieDMBuffer = movieDMBuffer;
    birecv->engine = engine;
    birecv->type = DMIC_RECEIVE;
    
    uspsema(burnSema);

    burnStack.push_back( birecv );
    burnStack.push_back( bisend );
    burnCount +=2;

    if ( pollfd != NULL )
	delete [] pollfd;
    pollfd = new struct pollfd[burnCount];

    int j = 0;
    list<BurnItem *>::iterator i;
    for(i=burnStack.begin();i!=burnStack.end();i++){
	if ( (*i)->type == DMIC_SEND ){
	    pollfd[j].fd = (*i)->movieDMBuffer->getDMICSourcePoolFD();
	    pollfd[j].events = POLLOUT;
	}
	if ( (*i)->type == DMIC_RECEIVE ){
	    pollfd[j].fd = (*i)->movieDMBuffer->getDMICDestPoolFD();
	    pollfd[j].events = POLLIN;
	}
	j++;
    }
    usvsema(burnSema);
}

void
SoDMBufferDMICMovieEngine::burn( void *){

    //--- Polling Loop ----------------------------------------------------------------
    //VLEvent event;
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
	    list<SoDMBufferDMICMovieEngine::BurnItem *>::iterator i;
	    for(i=burnStack.begin();i!=burnStack.end();i++){
		if ( pollfd[j].revents == POLLIN &&
		     (*i)->type == SoDMBufferDMICMovieEngine::DMIC_RECEIVE){
		    (*i)->movieDMBuffer->setValueFromDecoder();
		    (*i)->engine->trigger.setValue();
		}

		if ( pollfd[j].revents == POLLOUT &&
		     (*i)->type == SoDMBufferDMICMovieEngine::DMIC_SEND){
		    if ( (*i)->engine->needNextFrame() ){
			unsigned char *pixels = (*i)->movieDMBuffer->startEditingEncoder();
			size_t nbytes = (*i)->engine->movieGetNextField( pixels );
			(*i)->movieDMBuffer->finishEditingEncoder( nbytes );
		    }
		}
		j++;
	    }
	}
	usvsema(burnSema);
    }
}





