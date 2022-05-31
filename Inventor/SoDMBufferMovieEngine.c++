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
#include <Inventor/engines/SoDMBufferMovieEngine.h>

// System V
#include <stropts.h>

// Local
#include <Inventor/misc/checks.h>

SO_ENGINE_SOURCE( SoDMBufferMovieEngine );

void
SoDMBufferMovieEngine::initClass( void ){
    SO_ENGINE_INIT_CLASS( SoDMBufferMovieEngine, SoEngine, "Engine");
}
    
SoDMBufferMovieEngine::SoDMBufferMovieEngine(const char* movieFilename , int bufferCount)
{
    SO_ENGINE_CONSTRUCTOR( SoDMBufferMovieEngine );

    SO_ENGINE_ADD_INPUT( trigger, () );
    SO_ENGINE_ADD_OUTPUT( movieBits, SoSFDMBufferImage );
    SO_ENGINE_ADD_OUTPUT( scale, SoSFMatrix );

    lastFrameIndex = 0;
    openMovie( movieFilename, bufferCount );

    PlayAllFrames = False;

    frameRateSensor = new SoTimerSensor(doUpdate, this);
    frameRateSensor->setInterval(1.0/30.0);
    frameRateSensor->schedule();
}

SoDMBufferMovieEngine::~SoDMBufferMovieEngine(){
}

void
SoDMBufferMovieEngine::inputChanged(SoField *){
    evaluate();
}

void
SoDMBufferMovieEngine::evaluate(){
    trigger.getValue();
    SO_ENGINE_OUTPUT( scale, SoSFMatrix, setValue( movieMatrix ));
    SO_ENGINE_OUTPUT( movieBits, SoSFDMBufferImage, setValue( movieDMBuffer ));
}

void
SoDMBufferMovieEngine::stop(){
    frameRateSensor->unschedule();
}

void
SoDMBufferMovieEngine::start(){
    frameRateSensor->schedule();
}

void
SoDMBufferMovieEngine::doUpdate(void *data, SoSensor *){
    SoDMBufferMovieEngine *engine = (SoDMBufferMovieEngine *) data;
    engine->movieGetNextFrame();
    engine->trigger.setValue();
}

SbBool
SoDMBufferMovieEngine::openMovie( const char *filename, int bufferCount ){
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

  numFrames = mvGetTrackLength(mvtrackid) - 1;

  doDMBufferandMovieSetup( bufferCount );
  playMovie();

  return True;
}

void
SoDMBufferMovieEngine::closeMovie( void ){
  DMstatus status = DM_SUCCESS;
  
  if ( mvid != NULL){
      status = mvClose(mvid);
      CHECK_DM( status == DM_SUCCESS, "closeMovie() Error closing movie.\n");
  }

  mvid = NULL;
}

void
SoDMBufferMovieEngine::playMovie( void ){
  frameindex = 0;
  startTime = SbTime::getTimeOfDay();
}

void
SoDMBufferMovieEngine::stopMovie( void ){
}

void
SoDMBufferMovieEngine::rewindMovie( void ){
}

void
SoDMBufferMovieEngine::movieGetNextFrame( void ){
  //DMstatus status;

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
      return;

  lastFrameIndex = frameindex;

  movieDMBuffer->startEditing();
   //status = 
  mvRenderMovieToOpenGL( mvid, frameindex,  mvGetImageRate(mvtrackid));
//    glPushAttrib(GL_COLOR_BUFFER_BIT);
//    glBlendFunc(GL_ONE, GL_CONSTANT_ALPHA_EXT);
//    glBlendColorEXT(0,0,0,0.25);
//    glBlendEquationEXT(GL_FUNC_ADD_EXT);
//    glEnable(GL_BLEND);
//    glXMakeCurrent( movieDMBuffer->getDisplay(), movieDMBuffer->getPBuffer(), movieDMBuffer->getContext());
//    glCopyPixels(0,0,movieDMBuffer->getSize()[0],movieDMBuffer->getSize()[1],GL_COLOR);
//    glPopAttrib();
  movieDMBuffer->finishEditing();

}

void
SoDMBufferMovieEngine::doDMBufferandMovieSetup( int bufferCount ){

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

    Display *display = NULL;
    openStandardPBuffer( display, movieDMBuffer );
    movieDMBuffer->setBufferCount( bufferCount );
    movieDMBuffer->createPool();

}







