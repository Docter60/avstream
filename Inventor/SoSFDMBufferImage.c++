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



#include <Inventor/fields/SoSFDMBufferImage.h>

//////////////////////////////////////////////////////////////////////////////
//
// SoSFDMBufferImage class
//
//////////////////////////////////////////////////////////////////////////////

// Use most of the standard stuff:
SO_SFIELD_SOURCE(SoSFDMBufferImage, SbDMBuffer *, SbDMBuffer *);

void
SoSFDMBufferImage::initClass( void ){
    SO_SFIELD_INIT_CLASS( SoSFDMBufferImage, SoSField );
}

DMbuffer
SoSFDMBufferImage::getBuffer(SbVec2s &size )
{
    evaluate();

    // Get dmbuffer from connection!
    dmBuffer = getValue();

    size = dmBuffer->getSize();
    return dmBuffer->getValue();
}

const unsigned char *
SoSFDMBufferImage::getImage(SbVec2s &size, int &nc)
{
    evaluate();

    // Get dmbuffer from connection!
    dmBuffer = getValue();

    unsigned char *bytes; 
    return dmBuffer->getValue(size, nc, &bytes);
}

const unsigned char *
SoSFDMBufferImage::getImage( void )
{
    evaluate();

    // Get dmbuffer from connection!
    dmBuffer = getValue();

    unsigned char *bytes; 
    return dmBuffer->getValue(&bytes);
}

void
SoSFDMBufferImage::release( void ){
    // Get dmbuffer from connection!
    dmBuffer = getValue();

    dmBuffer->releaseValue();
}

unsigned char *
SoSFDMBufferImage::startEditing(SbVec2s &s, int &nc)
{
    // Get dmbuffer from connection!
    dmBuffer = getValue();

    return dmBuffer->startEditing(s,nc);
}

void
SoSFDMBufferImage::finishEditing()
{
    // Get dmbuffer from connection!
    dmBuffer = getValue();

    dmBuffer->finishEditing();
    valueChanged();
}

SbBool
SoSFDMBufferImage::readValue(SoInput *)
{
    return True;
}

void
SoSFDMBufferImage::writeValue(SoOutput *) const
{

}






