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



#ifndef  _SO_SF_DMBUFFER_IMAGE_H
#define  _SO_SF_DMBUFFER_IMAGE_H

// We can't really inherit from SoSFImage since it didn't
// bother to declare most of its functions virtual. May
// as well create a new class and avoid any mis-understanding.

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/fields/SoSubField.h>

#include <Inventor/SbDMBuffer.h>

class SoSFDMBufferImage: public SoSField  {

    SO_SFIELD_HEADER(SoSFDMBufferImage, SbDMBuffer *, SbDMBuffer *);

  public:

    // getValue returns the size, number of components and a 
    // pointer to a mapped dmbuffer image.
    const unsigned char *	getImage( void );
    const unsigned char *	getImage(SbVec2s &size, int &nc);
    DMbuffer			getBuffer( SbVec2s &size );
    void			release( void );

    unsigned char *		startEditing(SbVec2s &s, int &nc);
    void			finishEditing( void );

  SoINTERNAL public:
    static void			initClass();

  private:
    SbDMBuffer *	 	dmBuffer;

};

#endif







