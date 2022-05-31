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



#ifndef  _SB_DMBUFFER_H
#define  _SB_DMBUFFER_H

// Inventor
#include <Inventor/SbLinear.h>

// Digital Media
#include <dmedia/dmedia.h>
#include <dmedia/dm_buffer.h>
#include <dmedia/dm_imageconvert.h>
#include <vl/vl.h>

// GL
#include <GL/glu.h>
#include <GL/glx.h>

// System V
#include <ulocks.h>

class SbDMBuffer {

  public:

    SbDMBuffer( const SbVec2s &size );
    ~SbDMBuffer( void );

    // State Access	
    SbBool		 isLocked( void ) { return _LOCK_STATE; };

    unsigned char *	 getValue(SbVec2s &size, int &nc, unsigned char **pixels);
    unsigned char *	 getValue(unsigned char **pixels);

    DMbuffer  		 getValue(SbVec2s &size, int &nc);
    DMbuffer 		 getValue( void );

    void		 releaseValue( void );

    SbVec2s		 getSize( void ) { return _size; };
    DMparams *	 	 getFormat( void ) { return _imageFormat; };

    GLenum		 getGLFormat( void ) { return _GL_FORMAT; };
    GLenum		 getGLInternalFormat( void ) { return _GL_INTERNAL_FORMAT; };
    GLenum		 getGLDataType( void ) { return GL_UNSIGNED_BYTE; };

    // Modifying Content
    SbBool		 setValueFromVLEvent( VLEvent *event);
    unsigned char *	 startEditing( SbVec2s &size, int &nc);    // Map pixels for glDrawPixels, etc.
    DMbuffer		 startEditing( void ); 	     		   // Make current render context
    void       		 finishEditing( void );			   // Unmap / return context

    // Buffer access routines
    GLXFBConfigSGIX 	 getFBConfig( void ) { return _glConfig; };
    GLXContext 		 getContext( void )  { return _glContext; };
    GLXPbufferSGIX	 getPBuffer( void )  { return _glPBuffer; };
    Display *		 getDisplay( void )  { return _display; };

    // Constraints
    SbBool		 manage( Display *display, GLXFBConfigSGIX glConfig, GLXContext glContext, GLXPbufferSGIX glPBuffer);
    SbBool		 manage( VLServer server, VLPath path, VLNode drain);
    SbBool		 constrain( DMparams *imageFormat );
    SbBool		 constrain( VLServer server, VLPath path, VLNode node );
    SbBool		 constrain( int decoderDriver, DMparams *convParams, SbVec2s size);
    SbBool		 setBufferCount( int bufferCount );


    // DMIC
    int			 getDMICSourcePoolFD( void );
    int			 getDMICDestPoolFD( void );
    SbBool		 setValueFromDecoder( void );
    unsigned char *	 startEditingEncoder( void );
    void		 finishEditingEncoder( int nbytes );

    // Buffer Allocation
    SbBool		createPool( void );

  protected:
  private:
    // Internal
    SbVec2s		_size;		// Width and height of image
    SbBool		_LOCK_STATE;

    enum { UNLOCKED, LOCKED };
    void		 lock( void );
    void		 release( void );

    // Semaphores
    usptr_t *		_arena;
    usema_t *		_workBufferSema;

    // PBuffers
    GLXFBConfigSGIX 	_glConfig;
    GLXContext		_glContext;
    GLXPbufferSGIX	_glPBuffer;
    Display *	        _display;

    GLenum		_GL_FORMAT;
    GLenum		_GL_INTERNAL_FORMAT;

    // DMbuffers
    int			_bufferCount;
    DMbufferpool	_dmBufferPool;
    DMparams *		_poolParams;
    DMparams *          _imageFormat;    // Internal Image Format
    DMbuffer		_workBuffer;
    DMbuffer		_lockedBuffer;

    void		swapBuffers( void );

    // DMIC
    DMimageconverter	_dmICDecoder;
    DMbufferpool	_dmICInputBufferPool;
    DMbufferpool	_dmICOutputBufferPool;
    size_t		_dmICInputFrameSize;
    DMbuffer		_dmICInputBuffer;

};
#endif

// Convenience Routines:

// Video

void setFractControl(VLServer server, VLPath path, VLNode node, VLControlType type, int numerator, int denominator );
void setIntControl(VLServer server, VLPath path, VLNode node, VLControlType type, int newValue);
void setXYControl(VLServer server, VLPath path, VLNode node, VLControlType type, int x, int y );

#ifdef DEBUG
    void printParams( const DMparams* p, int indent );
    void printFBConfig(GLXFBConfigSGIX config);
    void printPBufferStats(GLXPbufferSGIX pbuffer);
#endif

Bool openStandardVideo( SbDMBuffer *prepBuffer, const char* deviceName, 
			SbVec2s origin, SbVec2s zoom, SbVec2s aspect,
			const VLServer &server, VLPath &path, VLNode &drain,
			VLControlValueType LinearOrGraphics);

Bool startVideo( VLServer server, VLPath path );
Bool stopVideo( VLServer server, VLPath path );
void closeStandardVideo( VLServer server);


// PBuffers
GLXFBConfigSGIX getFBConfig(int params[]);
GLXFBConfigSGIX setupDrawable(int params[], int w, int h, GLXPbufferSGIX* newDrawable);

Bool  openStandardPBuffer( Display *display, SbDMBuffer *prepBuffer);
Bool  openStandardDMICOutputPBuffer( SbDMBuffer *prepBuffer, SbVec2s size);
void closeStandardPBuffer();

// DMBuffers
unsigned char *map( DMbuffer & );
Bool makeReadable( SbDMBuffer &buffer );
